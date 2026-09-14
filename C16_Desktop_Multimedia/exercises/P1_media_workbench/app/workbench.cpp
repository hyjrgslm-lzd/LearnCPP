#include "workbench.hpp"

#include <c16/check.hpp>

#include <QApplication>
#include <QBoxLayout>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QSet>
#include <QTest>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace workbench {
namespace {

constexpr qint64 kMaxSessionBytes = 1024 * 1024;
constexpr double kMaxSafeInteger = 9007199254740991.0;

template<class T>
T readLe(const QByteArray& bytes, qsizetype offset)
{
    T value{};
    if (offset < 0 || offset > bytes.size() - qsizetype(sizeof(T))) {
        throw std::runtime_error("truncated media header");
    }
    std::memcpy(&value, bytes.constData() + offset, sizeof(T));
    return value;
}

QByteArray readBoundedFile(const QString& path, qint64 maxBytes = 8 * 1024 * 1024)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("cannot open media");
    }
    if (file.size() <= 0 || file.size() > maxBytes) {
        throw std::runtime_error("media size is outside exercise limits");
    }
    const QByteArray data = file.read(maxBytes + 1);
    if (data.size() != file.size()) {
        throw std::runtime_error("media read failed");
    }
    return data;
}

struct PcmSpan {
    qsizetype begin = 0;
    qsizetype bytes = 0;
};

struct AviAudioFormat {
    bool found = false;
    quint16 format = 0;
    quint16 channels = 0;
    quint32 sampleRate = 0;
    quint16 bits = 0;
};

qsizetype paddedEnd(qsizetype body, quint32 size)
{
    return body + qsizetype(size) + qsizetype(size % 2);
}

void validatePcm16Mono(quint16 format, quint16 channels, quint16 bits, quint32 rate)
{
    if (format != 1 || channels != 1 || bits != 16 || rate == 0) {
        throw std::runtime_error("only PCM16 mono audio is accepted in this exercise");
    }
}

void accumulatePcm16Mono(AnalysisResult& result, const QByteArray& data, qsizetype begin, qsizetype bytes,
    quint32 sampleRate, qint64 firstSample, qint64 totalSamples, const std::shared_ptr<std::atomic_bool>& cancel)
{
    if (sampleRate == 0 || totalSamples <= 0 || bytes <= 0) {
        throw std::runtime_error("missing pcm data");
    }
    if (bytes % 2 != 0) {
        throw std::runtime_error("pcm data has an odd trailing byte");
    }
    if (result.waveform.isEmpty()) {
        result.waveform.fill(0.0f, 64);
    }
    const qsizetype samples = bytes / 2;
    for (qsizetype i = 0; i < samples; ++i) {
        if (cancel && cancel->load(std::memory_order_acquire)) {
            result.error = "cancelled";
            return;
        }
        const qint16 sample = readLe<qint16>(data, begin + i * 2);
        const double value = std::abs(double(sample) / 32768.0);
        result.peak = std::max(result.peak, value);
        const qint64 absoluteSample = firstSample + i;
        const qsizetype bucket = std::min<qsizetype>(result.waveform.size() - 1,
            absoluteSample * result.waveform.size() / totalSamples);
        result.waveform[int(bucket)] = std::max(result.waveform[int(bucket)], float(value));
    }
    ++result.audioBuffers;
}

void finishPcm16Mono(AnalysisResult& result, const QByteArray& data, const QVector<PcmSpan>& spans,
    quint32 sampleRate, const std::shared_ptr<std::atomic_bool>& cancel)
{
    qint64 totalBytes = 0;
    for (const auto& span : spans) {
        if (span.bytes <= 0 || span.bytes % 2 != 0) {
            throw std::runtime_error("pcm data has an odd trailing byte");
        }
        totalBytes += span.bytes;
    }
    if (sampleRate == 0 || totalBytes <= 0) {
        throw std::runtime_error("missing pcm data");
    }
    const qint64 totalSamples = totalBytes / 2;
    result.durationMs = totalSamples * 1000 / sampleRate;
    qint64 firstSample = 0;
    for (const auto& span : spans) {
        accumulatePcm16Mono(result, data, span.begin, span.bytes, sampleRate, firstSample, totalSamples, cancel);
        if (!result.error.isEmpty()) {
            return;
        }
        firstSample += span.bytes / 2;
    }
}

AnalysisResult analyzeWav(const QString& id, const QString& path, int generation,
    const std::shared_ptr<std::atomic_bool>& cancel)
{
    const QByteArray data = readBoundedFile(path);
    if (data.size() < 44 || data.mid(0, 4) != "RIFF" || data.mid(8, 4) != "WAVE") {
        throw std::runtime_error("unsupported wav header");
    }

    qsizetype fmt = -1;
    qsizetype fmtBytes = 0;
    qsizetype pcm = -1;
    qsizetype pcmBytes = 0;
    for (qsizetype pos = 12; pos + 8 <= data.size();) {
        const QByteArray tag = data.mid(pos, 4);
        const quint32 size = readLe<quint32>(data, pos + 4);
        const qsizetype body = pos + 8;
        if (size > quint32(data.size()) || body > data.size() - qsizetype(size)) {
            throw std::runtime_error("truncated wav chunk");
        }
        if (tag == "fmt ") {
            fmt = body;
            fmtBytes = size;
        } else if (tag == "data") {
            pcm = body;
            pcmBytes = size;
            break;
        }
        pos = paddedEnd(body, size);
    }
    if (fmt < 0 || pcm < 0) {
        throw std::runtime_error("missing wav chunks");
    }
    if (fmtBytes < 16) {
        throw std::runtime_error("truncated wav fmt chunk");
    }
    const quint16 format = readLe<quint16>(data, fmt);
    const quint16 channels = readLe<quint16>(data, fmt + 2);
    const quint32 rate = readLe<quint32>(data, fmt + 4);
    const quint16 bits = readLe<quint16>(data, fmt + 14);
    validatePcm16Mono(format, channels, bits, rate);

    AnalysisResult result;
    result.generation = generation;
    result.id = id;
    result.path = path;
    finishPcm16Mono(result, data, QVector<PcmSpan>{{pcm, pcmBytes}}, rate, cancel);
    return result;
}

void parseAviStrl(const QByteArray& data, qsizetype begin, qsizetype end, AviAudioFormat& format)
{
    bool audioStream = false;
    AviAudioFormat candidate;
    for (qsizetype pos = begin; pos + 8 <= end;) {
        const QByteArray tag = data.mid(pos, 4);
        const quint32 size = readLe<quint32>(data, pos + 4);
        const qsizetype body = pos + 8;
        if (size > quint32(data.size()) || body > data.size() - qsizetype(size) || body + qsizetype(size) > end) {
            throw std::runtime_error("truncated avi stream chunk");
        }
        if (tag == "strh") {
            if (size < 4) {
                throw std::runtime_error("truncated avi stream header");
            }
            audioStream = data.mid(body, 4) == "auds";
        } else if (tag == "strf") {
            if (size < 16) {
                throw std::runtime_error("truncated avi audio format");
            }
            candidate.found = true;
            candidate.format = readLe<quint16>(data, body);
            candidate.channels = readLe<quint16>(data, body + 2);
            candidate.sampleRate = readLe<quint32>(data, body + 4);
            candidate.bits = readLe<quint16>(data, body + 14);
        }
        pos = paddedEnd(body, size);
    }
    if (audioStream && candidate.found) {
        format = candidate;
    }
}

void scanAviLists(const QByteArray& data, qsizetype begin, qsizetype end, AviAudioFormat& format,
    QVector<PcmSpan>& audioChunks, int& videoFrames)
{
    for (qsizetype pos = begin; pos + 8 <= end;) {
        const QByteArray tag = data.mid(pos, 4);
        const quint32 size = readLe<quint32>(data, pos + 4);
        const qsizetype body = pos + 8;
        if (size > quint32(data.size()) || body > data.size() - qsizetype(size) || body + qsizetype(size) > end) {
            throw std::runtime_error("truncated avi chunk");
        }
        if (tag == "LIST") {
            if (size < 4) {
                throw std::runtime_error("truncated avi list");
            }
            const QByteArray kind = data.mid(body, 4);
            const qsizetype childrenBegin = body + 4;
            const qsizetype childrenEnd = body + size;
            if (kind == "strl") {
                parseAviStrl(data, childrenBegin, childrenEnd, format);
            } else {
                scanAviLists(data, childrenBegin, childrenEnd, format, audioChunks, videoFrames);
            }
        } else if (tag == "00db") {
            ++videoFrames;
        } else if (tag == "01wb") {
            audioChunks.push_back({body, qsizetype(size)});
        }
        pos = paddedEnd(body, size);
    }
}

AnalysisResult analyzeAvi(const QString& id, const QString& path, int generation,
    const std::shared_ptr<std::atomic_bool>& cancel)
{
    const QByteArray data = readBoundedFile(path);
    if (data.size() < 12 || data.mid(0, 4) != "RIFF" || data.mid(8, 4) != "AVI ") {
        throw std::runtime_error("unsupported avi header");
    }

    AnalysisResult result;
    result.generation = generation;
    result.id = id;
    result.path = path;
    AviAudioFormat format;
    QVector<PcmSpan> audioChunks;
    scanAviLists(data, 12, data.size(), format, audioChunks, result.videoFrames);
    if (!format.found) {
        throw std::runtime_error("avi missing audio format");
    }
    validatePcm16Mono(format.format, format.channels, format.bits, format.sampleRate);
    if (result.videoFrames == 0 || audioChunks.isEmpty()) {
        throw std::runtime_error("avi fixture has no audio/video chunks");
    }
    finishPcm16Mono(result, data, audioChunks, format.sampleRate, cancel);
    return result;
}


template<class T>
void appendLe(QByteArray& out, T value)
{
    out.append(reinterpret_cast<const char*>(&value), int(sizeof(T)));
}

QByteArray makeChunk(const char tag[4], const QByteArray& body)
{
    QByteArray out(tag, 4);
    appendLe<quint32>(out, quint32(body.size()));
    out.append(body);
    if (body.size() % 2 != 0) {
        out.append('\0');
    }
    return out;
}

QByteArray makeList(const char kind[4], const QByteArray& children)
{
    QByteArray body(kind, 4);
    body.append(children);
    return makeChunk("LIST", body);
}

QByteArray makeRiff(const char kind[4], const QByteArray& children)
{
    QByteArray out("RIFF", 4);
    appendLe<quint32>(out, quint32(4 + children.size()));
    out.append(kind, 4);
    out.append(children);
    return out;
}

QByteArray pcmBytes(const QVector<qint16>& samples, bool oddTail = false)
{
    QByteArray out;
    out.reserve(samples.size() * 2 + (oddTail ? 1 : 0));
    for (qint16 sample : samples) {
        appendLe<qint16>(out, sample);
    }
    if (oddTail) {
        out.append('\x7f');
    }
    return out;
}

void writeAllBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(bytes) != bytes.size()) {
        throw std::runtime_error("cannot write oracle fixture");
    }
}

QVector<qint16> pulseSamples(int sampleRate, int ms)
{
    QVector<qint16> samples;
    samples.reserve(sampleRate * ms / 1000);
    const int total = sampleRate * ms / 1000;
    for (int i = 0; i < total; ++i) {
        samples.push_back((i / 64) % 2 == 0 ? qint16(20000) : qint16(-20000));
    }
    return samples;
}

void writeOracleWav(const QString& path, quint32 sampleRate, const QVector<qint16>& samples,
    bool extraChunk = false, bool badFmt = false, bool oddTail = false)
{
    QByteArray fmt;
    appendLe<quint16>(fmt, quint16(1));
    appendLe<quint16>(fmt, quint16(badFmt ? 2 : 1));
    appendLe<quint32>(fmt, sampleRate);
    appendLe<quint32>(fmt, sampleRate * quint32(badFmt ? 4 : 2));
    appendLe<quint16>(fmt, quint16(badFmt ? 4 : 2));
    appendLe<quint16>(fmt, quint16(16));
    QByteArray children;
    children.append(makeChunk("fmt ", fmt));
    if (extraChunk) {
        children.append(makeChunk("JUNK", QByteArray("metadata", 8)));
    }
    children.append(makeChunk("data", pcmBytes(samples, oddTail)));
    writeAllBytes(path, makeRiff("WAVE", children));
}

void writeOracleAvi(const QString& path, quint32 sampleRate, const QVector<qint16>& samples, int splitSamples)
{
    QByteArray strh("auds", 4);
    strh.append(QByteArray(52, '\0'));
    QByteArray strf;
    appendLe<quint16>(strf, quint16(1));
    appendLe<quint16>(strf, quint16(1));
    appendLe<quint32>(strf, sampleRate);
    appendLe<quint32>(strf, sampleRate * 2);
    appendLe<quint16>(strf, quint16(2));
    appendLe<quint16>(strf, quint16(16));
    QByteArray hdrl;
    hdrl.append(makeList("strl", makeChunk("strh", strh) + makeChunk("strf", strf)));

    QByteArray movi;
    movi.append(makeChunk("00db", QByteArray(16, '\x11')));
    for (int first = 0; first < samples.size(); first += splitSamples) {
        QVector<qint16> slice;
        const int count = std::min(splitSamples, int(samples.size() - first));
        slice.reserve(count);
        for (int i = 0; i < count; ++i) slice.push_back(samples[first + i]);
        movi.append(makeChunk("01wb", pcmBytes(slice)));
    }
    writeAllBytes(path, makeRiff("AVI ", makeList("hdrl", hdrl) + makeList("movi", movi)));
}

double waveformMax(const MediaListModel* model)
{
    const auto* item = model->itemById(model->selectedId());
    if (!item) return 0.0;
    double value = 0.0;
    for (float bucket : item->waveform) value = std::max(value, double(bucket));
    return value;
}


bool saveWidgetSnapshot(QWidget* window, const QString& path)
{
    if (!window || window->size().isEmpty()) {
        return false;
    }
    QImage image(window->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    window->render(&painter);
    painter.end();
    return image.save(path);
}

void waitFor(std::function<bool()> done, int timeoutMs, QString message)
{
    QElapsedTimer timer;
    timer.start();
    while (!done() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QTest::qWait(20);
    }
    c16::require(done(), message.toStdString());
}

} // namespace

MediaListModel::MediaListModel(QObject* parent) : QAbstractListModel(parent) {}

int MediaListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : visible_.size();
}

QVariant MediaListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= visible_.size()) {
        return {};
    }
    const MediaItem& item = items_[visible_[index.row()]];
    switch (role) {
    case IdRole: return item.id;
    case PathRole: return item.path;
    case DisplayNameRole:
    case Qt::DisplayRole: return item.displayName;
    case ErrorRole: return item.error;
    case DurationRole: return item.durationMs;
    case PeakRole: return item.peak;
    case SelectedRole: return item.id == selectedId_;
    case RowRole: return index.row();
    default: return {};
    }
}

QHash<int, QByteArray> MediaListModel::roleNames() const
{
    return {{IdRole, "mediaId"}, {PathRole, "path"}, {DisplayNameRole, "displayName"},
        {ErrorRole, "error"}, {DurationRole, "durationMs"}, {PeakRole, "peak"},
        {SelectedRole, "selected"}, {RowRole, "row"}};
}

int MediaListModel::addFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return -1;
    }
    const QString id = makeId(info.absoluteFilePath());
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == id) {
            rebuildFilter();
            return visible_.indexOf(i);
        }
    }
    beginResetModel();
    items_.push_back({id, info.absoluteFilePath(), info.fileName()});
    rebuildFilter();
    endResetModel();
    return visible_.indexOf(items_.size() - 1);
}

void MediaListModel::setFilter(const QString& text)
{
    beginResetModel();
    filter_ = text;
    rebuildFilter();
    endResetModel();
}

QString MediaListModel::filter() const { return filter_; }

bool MediaListModel::selectRow(int row)
{
    if (row < 0 || row >= visible_.size()) {
        return false;
    }
    selectedId_ = items_[visible_[row]].id;
    emit dataChanged(index(0), index(std::max(0, rowCount() - 1)), {SelectedRole});
    return true;
}

QString MediaListModel::selectedId() const { return selectedId_; }

QString MediaListModel::selectedPath() const
{
    if (const auto* item = itemById(selectedId_)) {
        return item->path;
    }
    return {};
}

QString MediaListModel::selectedName() const
{
    if (const auto* item = itemById(selectedId_)) {
        return item->displayName;
    }
    return {};
}

void MediaListModel::applyAnalysis(const AnalysisResult& result)
{
    for (int row = 0; row < items_.size(); ++row) {
        if (items_[row].id == result.id) {
            items_[row].error = result.error;
            items_[row].durationMs = result.durationMs;
            items_[row].peak = result.peak;
            items_[row].waveform = result.waveform;
            const int visibleRow = visible_.indexOf(row);
            if (visibleRow >= 0) {
                emit dataChanged(index(visibleRow), index(visibleRow), {ErrorRole, DurationRole, PeakRole});
            }
            return;
        }
    }
}

QJsonObject MediaListModel::toJson() const
{
    QJsonArray items;
    for (const auto& item : items_) {
        items.append(QJsonObject{{"path", item.path}, {"id", item.id}, {"name", item.displayName}});
    }
    return {{"items", items}, {"selected", selectedId_}, {"filter", filter_}, {"version", 1}};
}

bool MediaListModel::restore(const QJsonObject& object)
{
    QVector<MediaItem> items;
    QVector<int> visible;
    QString selected;
    QString filter;
    if (!prepareRestore(object, items, visible, selected, filter, nullptr)) {
        return false;
    }
    beginResetModel();
    items_ = std::move(items);
    visible_ = std::move(visible);
    selectedId_ = std::move(selected);
    filter_ = std::move(filter);
    endResetModel();
    return true;
}

bool MediaListModel::prepareRestore(const QJsonObject& object, QVector<MediaItem>& items,
    QVector<int>& visible, QString& selected, QString& filter, QString* error) const
{
    if (object.value("version").toInt(-1) != 1 || !object.value("items").isArray()) {
        if (error) *error = "unsupported session version";
        return false;
    }
    for (const auto value : object.value("items").toArray()) {
        if (!value.isObject()) {
            if (error) *error = "invalid media row";
            return false;
        }
        const auto item = value.toObject();
        const QString path = item.value("path").toString();
        if (path.isEmpty()) {
            if (error) *error = "missing media path";
            return false;
        }
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            if (error) *error = "session references missing media";
            return false;
        }
        const QString id = makeId(info.absoluteFilePath());
        const QString savedId = item.value("id").toString();
        if (!savedId.isEmpty() && savedId != id) {
            if (error) *error = "media id does not match path";
            return false;
        }
        items.push_back({id, info.absoluteFilePath(), info.fileName()});
    }
    filter = object.value("filter").toString();
    for (int i = 0; i < items.size(); ++i) {
        if (filter.isEmpty() || items[i].displayName.contains(filter, Qt::CaseInsensitive)
            || items[i].path.contains(filter, Qt::CaseInsensitive)) {
            visible.push_back(i);
        }
    }
    selected = object.value("selected").toString();
    if (!selected.isEmpty()) {
        bool found = false;
        for (const auto& item : items) {
            found = found || item.id == selected;
        }
        if (!found) {
            if (error) *error = "selected media id is not in session";
            return false;
        }
    }
    return true;
}

const MediaItem* MediaListModel::itemById(const QString& id) const
{
    for (const auto& item : items_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

QString MediaListModel::makeId(const QString& path)
{
    return QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toHex();
}

void MediaListModel::rebuildFilter()
{
    visible_.clear();
    for (int i = 0; i < items_.size(); ++i) {
        if (matches(items_[i])) {
            visible_.push_back(i);
        }
    }
}

bool MediaListModel::matches(const MediaItem& item) const
{
    return filter_.isEmpty() || item.displayName.contains(filter_, Qt::CaseInsensitive)
        || item.path.contains(filter_, Qt::CaseInsensitive);
}

AnalysisResult AnalysisWorker::analyze(QString id, QString path, int generation,
    std::shared_ptr<std::atomic_bool> cancel)
{
    try {
        const QString suffix = QFileInfo(path).suffix().toLower();
        if (suffix == "wav") {
            return analyzeWav(id, path, generation, cancel);
        }
        if (suffix == "avi") {
            return analyzeAvi(id, path, generation, cancel);
        }
        throw std::runtime_error("unsupported media format");
    } catch (const std::exception& e) {
        return {generation, id, path, QString::fromUtf8(e.what())};
    }
}

SessionController::SessionController(QObject* parent) : QObject(parent)
{
    worker_ = new AnalysisWorker;
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.start();
    player_.setAudioOutput(&audio_);
    player_.setAudioBufferOutput(&audioTap_);
    connect(&audioTap_, &QAudioBufferOutput::audioBufferReceived, this, [this](const QAudioBuffer& buffer) {
        if (buffer.frameCount() > 0) {
            ++audioBuffers_;
        }
    });
}

SessionController::~SessionController() { close(); }

MediaListModel* SessionController::model() { return &model_; }
QMediaPlayer* SessionController::player() { return &player_; }
QVideoSink* SessionController::videoSink() const { return videoWidget_ ? videoWidget_->videoSink() : nullptr; }

bool SessionController::addMedia(const QString& path) { return model_.addFile(path) >= 0; }

bool SessionController::selectRow(int row)
{
    if (!model_.selectRow(row)) {
        return false;
    }
    player_.setSource(QUrl::fromLocalFile(model_.selectedPath()));
    startAnalysis();
    emit stateChanged();
    return true;
}

void SessionController::setFilter(const QString& text) { model_.setFilter(text); }

void SessionController::togglePlayback()
{
    if (player_.playbackState() == QMediaPlayer::PlayingState) {
        player_.pause();
    } else {
        player_.play();
    }
    emit stateChanged();
}

void SessionController::seek(int ms) { player_.setPosition(ms); }

bool SessionController::addMarker(qint64 ms, const QString& label)
{
    if (model_.selectedId().isEmpty() || ms < 0 || label.isEmpty()) {
        return false;
    }
    markers_.append(QJsonObject{{"id", model_.selectedId()}, {"ms", double(ms)}, {"label", label}});
    emit stateChanged();
    return true;
}

bool SessionController::saveSession(const QString& path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError("cannot save session");
        return false;
    }
    QJsonObject root = model_.toJson();
    root["markers"] = markers_;
    const QByteArray payload = QJsonDocument(root).toJson();
    if (payload.size() > kMaxSessionBytes) {
        setError("session is too large");
        return false;
    }
    if (file.write(payload) != payload.size()) {
        setError("cannot write session");
        return false;
    }
    if (!file.commit()) {
        setError("cannot commit session");
        return false;
    }
    return true;
}

bool SessionController::restoreSession(const QString& path)
{
    SessionSnapshot snapshot;
    QString error;
    if (!parseSessionFile(path, snapshot, &error)) {
        setError(error);
        return false;
    }
    applySessionSnapshot(snapshot);
    if (!model_.selectedId().isEmpty()) {
        player_.setSource(QUrl::fromLocalFile(model_.selectedPath()));
        startAnalysis();
    }
    return true;
}

QString SessionController::selectedName() const { return model_.selectedName(); }
QString SessionController::errorText() const { return error_; }
bool SessionController::playing() const { return player_.playbackState() == QMediaPlayer::PlayingState; }
int SessionController::acceptedGeneration() const { return acceptedGeneration_; }
int SessionController::decodedAudioBuffers() const { return audioBuffers_; }
int SessionController::decodedVideoFrames() const { return videoFrames_; }
double SessionController::selectedPeak() const
{
    if (const auto* item = model_.itemById(model_.selectedId())) return item->peak;
    return 0.0;
}
qint64 SessionController::selectedDurationMs() const
{
    if (const auto* item = model_.itemById(model_.selectedId())) return item->durationMs;
    return 0;
}

QString SessionController::markerSummary() const
{
    if (markers_.isEmpty()) {
        return "Markers: 0";
    }
    const auto last = markers_.last().toObject();
    return QString("Markers: %1 | %2 ms | %3")
        .arg(markers_.size())
        .arg(qint64(last.value("ms").toDouble()))
        .arg(last.value("label").toString());
}

void SessionController::attachVideo(QVideoWidget* widget)
{
    videoWidget_ = widget;
    player_.setVideoOutput(widget);
    if (auto* sink = widget->videoSink()) {
        connect(sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
            if (frame.isValid()) {
                ++videoFrames_;
            }
        });
    }
}

void SessionController::close()
{
    if (closing_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    cancel_->store(true, std::memory_order_release);
    player_.stop();
    workerThread_.quit();
    workerThread_.wait();
}

void SessionController::startAnalysis()
{
    cancel_->store(true, std::memory_order_release);
    cancel_ = std::make_shared<std::atomic_bool>(false);
    const int gen = generation_.fetch_add(1, std::memory_order_acq_rel) + 1;
    const QString id = model_.selectedId();
    const QString path = model_.selectedPath();
    QPointer<SessionController> self(this);
    auto cancel = cancel_;
    QMetaObject::invokeMethod(worker_, [self, id, path, gen, cancel] {
        if (!self) {
            return;
        }
        AnalysisResult result = self->worker_->analyze(id, path, gen, cancel);
        QMetaObject::invokeMethod(self, [self, result] {
            if (self) {
                self->acceptAnalysis(result);
            }
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void SessionController::acceptAnalysis(const AnalysisResult& result)
{
    if (closing_.load(std::memory_order_acquire) || result.generation != generation_.load(std::memory_order_acquire)) {
        return;
    }
    acceptedGeneration_ = result.generation;
    model_.applyAnalysis(result);
    setError(result.error);
    emit stateChanged();
}

void SessionController::setError(QString error)
{
    error_ = std::move(error);
    emit stateChanged();
}

bool SessionController::parseSessionFile(const QString& path, SessionSnapshot& snapshot, QString* error) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "cannot open session";
        return false;
    }
    if (file.size() > kMaxSessionBytes) {
        if (error) *error = "session is too large";
        return false;
    }
    const QByteArray payload = file.read(kMaxSessionBytes + 1);
    if (payload.size() != file.size()) {
        if (error) *error = "cannot read session";
        return false;
    }
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = "corrupt session json";
        return false;
    }
    const QJsonObject root = doc.object();
    if (!model_.prepareRestore(root, snapshot.items, snapshot.visible, snapshot.selected, snapshot.filter, error)) {
        return false;
    }
    QSet<QString> ids;
    for (const auto& item : snapshot.items) ids.insert(item.id);
    if (!root.value("markers").isArray()) {
        if (error) *error = "session markers must be an array";
        return false;
    }
    for (const auto value : root.value("markers").toArray()) {
        if (!value.isObject()) {
            if (error) *error = "invalid session marker";
            return false;
        }
        const auto marker = value.toObject();
        const QString id = marker.value("id").toString();
        const double ms = marker.value("ms").toDouble(-1);
        if (!ids.contains(id) || !marker.value("ms").isDouble() || !std::isfinite(ms) || ms < 0
            || ms > kMaxSafeInteger || std::floor(ms) != ms || !marker.value("label").isString()) {
            if (error) *error = "invalid session marker";
            return false;
        }
        snapshot.markers.append(marker);
    }
    return true;
}

void SessionController::applySessionSnapshot(const SessionSnapshot& snapshot)
{
    QJsonArray items;
    for (const auto& item : snapshot.items) items.append(QJsonObject{{"id", item.id}, {"path", item.path}});
    model_.restore(QJsonObject{{"version", 1}, {"items", items}, {"selected", snapshot.selected}, {"filter", snapshot.filter}});
    markers_ = snapshot.markers;
    emit stateChanged();
}

class WaveformWidget final : public QWidget {
public:
    explicit WaveformWidget(MediaListModel& model, QWidget* parent = nullptr) : QWidget(parent), model_(model)
    {
        setMinimumHeight(72);
        setAccessibleName("Waveform preview");
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(20, 24, 30));
        p.setPen(QColor(70, 170, 220));
        const auto* item = model_.itemById(model_.selectedId());
        if (!item || item->waveform.isEmpty()) {
            p.drawText(rect(), Qt::AlignCenter, "No waveform");
            return;
        }
        const int mid = height() / 2;
        for (int i = 0; i < item->waveform.size(); ++i) {
            const int x = i * width() / item->waveform.size();
            const int h = int(item->waveform[i] * mid);
            p.drawLine(x, mid - h, x, mid + h);
        }
    }

private:
    MediaListModel& model_;
};

QWidget* createWorkbenchWindow(SessionController& controller)
{
    auto* window = new QWidget;
    window->setObjectName("workbenchWindow");
    window->setWindowTitle("C16 MediaWorkbench");
    window->setAccessibleName("MediaWorkbench window");
    auto* root = new QVBoxLayout(window);
    auto* filter = new QLineEdit;
    filter->setObjectName("filterEdit");
    filter->setPlaceholderText("Filter media");
    filter->setAccessibleName("Media filter");
    auto* list = new QListView;
    list->setObjectName("mediaList");
    list->setModel(controller.model());
    list->setAccessibleName("Media list");
    auto* video = new QVideoWidget;
    video->setObjectName("videoPreview");
    video->setMinimumSize(320, 180);
    video->setAccessibleName("Video preview");
    controller.attachVideo(video);
    auto* waveform = new WaveformWidget(*controller.model());
    waveform->setObjectName("waveformPreview");
    auto* seek = new QSlider(Qt::Horizontal);
    seek->setObjectName("seekSlider");
    seek->setAccessibleName("Seek position");
    seek->setRange(0, 1000);
    auto* error = new QLabel;
    error->setObjectName("errorLabel");
    error->setAccessibleName("Current error");
    auto* markers = new QLabel(controller.markerSummary());
    markers->setObjectName("markerLabel");
    markers->setAccessibleName("Timestamp markers");
    auto* buttons = new QHBoxLayout;
    auto* open = new QPushButton("Open");
    auto* play = new QPushButton("Play/Pause");
    auto* mark = new QPushButton("Mark");
    auto* save = new QPushButton("Save");
    auto* restore = new QPushButton("Restore");
    open->setObjectName("openButton");
    play->setObjectName("playButton");
    mark->setObjectName("markerButton");
    save->setObjectName("saveButton");
    restore->setObjectName("restoreButton");
    open->setAccessibleName("Open media files");
    play->setAccessibleName("Play or pause");
    mark->setAccessibleName("Add timestamp marker");
    save->setAccessibleName("Save session");
    restore->setAccessibleName("Restore session");
    buttons->addWidget(open);
    buttons->addWidget(play);
    buttons->addWidget(mark);
    buttons->addWidget(save);
    buttons->addWidget(restore);
    root->addWidget(filter);
    root->addWidget(list);
    root->addWidget(video);
    root->addWidget(waveform);
    root->addWidget(seek);
    root->addWidget(error);
    root->addWidget(markers);
    root->addLayout(buttons);

    QObject::connect(filter, &QLineEdit::textChanged, &controller, &SessionController::setFilter);
    QObject::connect(list, &QListView::clicked, &controller, [&controller, waveform](const QModelIndex& index) {
        controller.selectRow(index.row());
        waveform->update();
    });
    QObject::connect(controller.model(), &QAbstractItemModel::dataChanged, waveform, [waveform] { waveform->update(); });
    QObject::connect(open, &QPushButton::clicked, window, [window, &controller] {
        for (const QString& path : QFileDialog::getOpenFileNames(window, "Open media")) {
            controller.addMedia(path);
        }
    });
    QObject::connect(play, &QPushButton::clicked, &controller, &SessionController::togglePlayback);
    QObject::connect(seek, &QSlider::valueChanged, &controller, &SessionController::seek);
    QObject::connect(mark, &QPushButton::clicked, &controller, [&controller, markers] {
        const QString label = QString("marker %1").arg(controller.markerSummary().section(' ', 1, 1).section('|', 0, 0).toInt() + 1);
        if (controller.addMarker(controller.player()->position(), label)) {
            markers->setText(controller.markerSummary());
        }
    });
    QObject::connect(save, &QPushButton::clicked, window, [&controller] {
        controller.saveSession(QDir(QDir::tempPath()).filePath("c16-workbench-session.json"));
    });
    QObject::connect(restore, &QPushButton::clicked, window, [&controller] {
        controller.restoreSession(QDir(QDir::tempPath()).filePath("c16-workbench-session.json"));
    });
    QObject::connect(&controller, &SessionController::stateChanged, error, [&controller, error, waveform, markers] {
        error->setText(controller.errorText());
        markers->setText(controller.markerSummary());
        waveform->update();
    });
    return window;
}

int runWorkbenchSelfCheck(const QString& fixtureDir, const QString& snapshotPath)
{
    return c16::run([&] {
        SessionController controller;
        auto window = std::unique_ptr<QWidget>(createWorkbenchWindow(controller));
        window->resize(720, 520);
        window->show();
        auto* filter = window->findChild<QLineEdit*>("filterEdit");
        auto* list = window->findChild<QListView*>("mediaList");
        auto* play = window->findChild<QPushButton*>("playButton");
        auto* mark = window->findChild<QPushButton*>("markerButton");
        auto* saveButton = window->findChild<QPushButton*>("saveButton");
        auto* restoreButton = window->findChild<QPushButton*>("restoreButton");
        auto* seek = window->findChild<QSlider*>("seekSlider");
        auto* markerLabel = window->findChild<QLabel*>("markerLabel");
        c16::require(filter && list && play && mark && saveButton && restoreButton && seek && markerLabel,
            "interactive controls exist");
        const QString emptySession = QDir(fixtureDir).filePath("empty-session.json");
        c16::require(controller.saveSession(emptySession), "empty app session saves");
        c16::require(controller.restoreSession(emptySession), "empty app session restores");
        c16::require(controller.model()->rowCount() == 0 && controller.markerSummary() == "Markers: 0",
            "empty app session round-trips without stale rows or markers");

        const QString wav = QDir(fixtureDir).filePath("c16_pulse_mono_48k_s16.wav");
        const QString avi = QDir(fixtureDir).filePath("c16_rgb24_pcm_1s.avi");
        const QString empty = QDir(fixtureDir).filePath("c16_empty.media");
        const QString truncated = QDir(fixtureDir).filePath("c16_truncated.avi");
        const QVector<qint16> oracleSamples = pulseSamples(24000, 500);
        const QString oracleWav = QDir(fixtureDir).filePath("oracle_24k.wav");
        const QString oracleExtraWav = QDir(fixtureDir).filePath("oracle_24k_extra.wav");
        const QString oracleAvi = QDir(fixtureDir).filePath("oracle_24k_split.avi");
        const QString oracleBadFmt = QDir(fixtureDir).filePath("oracle_bad_fmt.wav");
        const QString oracleOddTail = QDir(fixtureDir).filePath("oracle_odd_tail.wav");
        writeOracleWav(oracleWav, 24000, oracleSamples);
        writeOracleWav(oracleExtraWav, 24000, oracleSamples, true);
        writeOracleAvi(oracleAvi, 24000, oracleSamples, 1111);
        writeOracleWav(oracleBadFmt, 24000, oracleSamples, false, true);
        writeOracleWav(oracleOddTail, 24000, oracleSamples, false, false, true);
        c16::require(controller.addMedia(wav), "wav fixture imports");
        c16::require(controller.addMedia(avi), "avi fixture imports");
        c16::require(controller.addMedia(empty), "empty fixture imports for error path");
        c16::require(controller.addMedia(truncated), "truncated fixture imports for error path");
        c16::require(controller.addMedia(oracleWav), "oracle wav imports");
        c16::require(controller.addMedia(oracleExtraWav), "oracle extra chunk wav imports");
        c16::require(controller.addMedia(oracleAvi), "oracle split avi imports");
        c16::require(controller.addMedia(oracleBadFmt), "oracle bad fmt imports");
        c16::require(controller.addMedia(oracleOddTail), "oracle odd tail imports");
        c16::require(controller.model()->rowCount() == 9, "model lists imported media and oracle fixtures");
        QTest::mouseClick(filter, Qt::LeftButton);
        QTest::keyClicks(filter, "pulse");
        c16::require(controller.model()->rowCount() == 1, "filter narrows list");
        filter->clear();
        c16::require(controller.model()->rowCount() == 9, "clearing filter restores all rows");
        const QModelIndex first = controller.model()->index(0);
        QTest::mouseClick(list->viewport(), Qt::LeftButton, {}, list->visualRect(first).center());
        c16::require(controller.selectedName().contains("pulse"), "list click selects first row");
        waitFor([&] { return controller.acceptedGeneration() >= 1; }, 3000, "wav analysis finishes");
        c16::require(controller.errorText().isEmpty(), "wav analysis has no error");
        c16::require(controller.selectedPeak() > 0.60 && controller.selectedPeak() < 0.80,
            "wav analysis computes peak from PCM samples");
        c16::require(controller.selectedDurationMs() == 1000, "wav analysis computes duration from PCM samples");
        QTest::mouseClick(mark, Qt::LeftButton);
        c16::require(markerLabel->text().contains("Markers: 1") && markerLabel->text().contains("ms")
                && markerLabel->text().contains("marker 1"),
            "marker button shows controller marker time and label");

        const QString session = QDir(fixtureDir).filePath("session.json");
        c16::require(controller.saveSession(session), "session saves with QSaveFile");
        QTest::mouseClick(saveButton, Qt::LeftButton);
        QTest::mouseClick(restoreButton, Qt::LeftButton);
        SessionController restored;
        c16::require(restored.restoreSession(session), "session restores existing media");
        c16::require(restored.model()->rowCount() == 9, "restored session keeps media rows");

        const int rowsBeforeBadRestore = controller.model()->rowCount();
        const QString badJson = QDir(fixtureDir).filePath("bad.json");
        QFile badJsonFile(badJson);
        c16::require(badJsonFile.open(QIODevice::WriteOnly), "bad json can be written");
        badJsonFile.write("{not json");
        badJsonFile.close();
        c16::require(!controller.restoreSession(badJson), "corrupt session is rejected");
        c16::require(controller.model()->rowCount() == rowsBeforeBadRestore, "corrupt restore keeps old state");
        const QString wrongVersion = QDir(fixtureDir).filePath("wrong-version.json");
        QFile wrongFile(wrongVersion);
        c16::require(wrongFile.open(QIODevice::WriteOnly), "wrong version can be written");
        wrongFile.write(QJsonDocument(QJsonObject{{"version", 99}, {"items", QJsonArray{}}, {"markers", QJsonArray{}}}).toJson());
        wrongFile.close();
        c16::require(!controller.restoreSession(wrongVersion), "unknown session version is rejected");
        const QString badSelected = QDir(fixtureDir).filePath("bad-selected.json");
        QFile::remove(badSelected);
        c16::require(controller.saveSession(badSelected), "base session for selected mutation saves");
        QFile selectedFile(badSelected);
        c16::require(selectedFile.open(QIODevice::ReadOnly), "selected mutation source opens");
        auto selectedDoc = QJsonDocument::fromJson(selectedFile.readAll()).object();
        selectedFile.close();
        selectedDoc["selected"] = "missing";
        c16::require(selectedFile.open(QIODevice::WriteOnly | QIODevice::Truncate), "selected mutation target opens");
        selectedFile.write(QJsonDocument(selectedDoc).toJson());
        selectedFile.close();
        c16::require(!controller.restoreSession(badSelected), "invalid selected id is rejected");
        const QString badMarker = QDir(fixtureDir).filePath("bad-marker.json");
        QFile::remove(badMarker);
        c16::require(controller.saveSession(badMarker), "base session for marker mutation saves");
        QFile markerFile(badMarker);
        c16::require(markerFile.open(QIODevice::ReadOnly), "marker mutation source opens");
        auto markerDoc = QJsonDocument::fromJson(markerFile.readAll()).object();
        markerFile.close();
        markerDoc["markers"] = QJsonArray{QJsonObject{{"id", "missing"}, {"ms", -1}, {"label", "bad"}}};
        c16::require(markerFile.open(QIODevice::WriteOnly | QIODevice::Truncate), "marker mutation target opens");
        markerFile.write(QJsonDocument(markerDoc).toJson());
        markerFile.close();
        c16::require(!controller.restoreSession(badMarker), "invalid marker is rejected");

        const int beforeAvi = controller.acceptedGeneration();
        c16::require(controller.selectRow(1), "selection accepts video row");
        waitFor([&] { return controller.acceptedGeneration() > beforeAvi; }, 3000, "avi analysis finishes");
        c16::require(controller.selectedPeak() > 0.60 && controller.selectedDurationMs() == 1000,
            "avi analysis derives waveform and duration from PCM chunks");
        const double fixtureAviPeak = controller.selectedPeak();
        const qint64 fixtureAviDuration = controller.selectedDurationMs();
        const double fixtureAviWaveMax = waveformMax(controller.model());
        c16::require(fixtureAviWaveMax > 0.60, "avi waveform uses global PCM samples");

        const int beforeOracleWav = controller.acceptedGeneration();
        c16::require(controller.selectRow(4), "oracle 24k wav row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeOracleWav; }, 3000, "oracle wav analysis finishes");
        const double oraclePeak = controller.selectedPeak();
        const qint64 oracleDuration = controller.selectedDurationMs();
        const double oracleWaveMax = waveformMax(controller.model());
        c16::require(oracleDuration == 500 && oraclePeak > 0.55 && oraclePeak < 0.70 && oracleWaveMax > 0.55,
            "oracle wav uses declared 24k sample rate and real PCM peak");

        const int beforeExtraWav = controller.acceptedGeneration();
        c16::require(controller.selectRow(5), "oracle extra-chunk wav row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeExtraWav; }, 3000, "extra wav analysis finishes");
        c16::require(controller.selectedDurationMs() == oracleDuration && std::abs(controller.selectedPeak() - oraclePeak) < 0.001,
            "extra RIFF chunks do not change wav analysis");

        const int beforeOracleAvi = controller.acceptedGeneration();
        c16::require(controller.selectRow(6), "oracle split avi row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeOracleAvi; }, 3000, "oracle avi analysis finishes");
        c16::require(controller.selectedDurationMs() == oracleDuration && std::abs(controller.selectedPeak() - oraclePeak) < 0.001
                && std::abs(waveformMax(controller.model()) - oracleWaveMax) < 0.001,
            "same PCM in split AVI and WAV produces matching duration peak and waveform scale");
        c16::require(fixtureAviDuration == 1000 && fixtureAviPeak > 0.60, "48k fixture remains a separate media oracle");
        const int beforePlaybackAvi = controller.acceptedGeneration();
        c16::require(controller.selectRow(1), "real video fixture is selected before playback check");
        waitFor([&] { return controller.acceptedGeneration() > beforePlaybackAvi; }, 3000, "playback avi analysis refreshes");
        QTest::mouseClick(play, Qt::LeftButton);
        waitFor([&] { return controller.player()->mediaStatus() != QMediaPlayer::NoMedia; }, 3000, "player accepts source");
        waitFor([&] { return controller.decodedAudioBuffers() > 0 && controller.decodedVideoFrames() > 0; },
            5000, "Qt media backend emits decoded audio buffers and video frames");
        seek->setValue(250);
        QTest::mouseClick(play, Qt::LeftButton);
        const int beforeBadFmt = controller.acceptedGeneration();
        c16::require(controller.selectRow(7), "bad fmt media row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeBadFmt && controller.errorText().contains("PCM16 mono"); },
            3000, "bad fmt wav is rejected");
        const int beforeOddTail = controller.acceptedGeneration();
        c16::require(controller.selectRow(8), "odd tail media row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeOddTail && controller.errorText().contains("odd"); },
            3000, "odd pcm tail is rejected");

        const int beforeEmpty = controller.acceptedGeneration();
        c16::require(controller.selectRow(2), "empty media row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeEmpty && !controller.errorText().isEmpty(); },
            3000, "empty media analysis reports error");
        const int beforeTruncated = controller.acceptedGeneration();
        c16::require(controller.selectRow(3), "truncated media row can be selected");
        waitFor([&] { return controller.acceptedGeneration() > beforeTruncated
                && (controller.errorText().contains("truncated") || controller.errorText().contains("unsupported")); },
            3000, "truncated media analysis reports format error");
        const int beforeFinalWav = controller.acceptedGeneration();
        c16::require(controller.selectRow(0), "wav row can be reselected for final preview");
        waitFor([&] { return controller.acceptedGeneration() > beforeFinalWav && controller.selectedPeak() > 0.60; },
            3000, "final wav analysis restores visible waveform state");

        if (!snapshotPath.isEmpty()) {
            c16::require(saveWidgetSnapshot(window.get(), snapshotPath), "snapshot writes window grab");
        }
        controller.close();
        restored.close();
    });
}


int runWorkbenchSnapshot(const QString& fixtureDir, const QString& snapshotPath)
{
    return c16::run([&] {
        SessionController controller;
        auto window = std::unique_ptr<QWidget>(createWorkbenchWindow(controller));
        window->resize(720, 520);
        window->show();
        c16::require(controller.addMedia(QDir(fixtureDir).filePath("c16_pulse_mono_48k_s16.wav")),
            "snapshot wav imports");
        c16::require(controller.addMedia(QDir(fixtureDir).filePath("c16_rgb24_pcm_1s.avi")),
            "snapshot avi imports");
        c16::require(controller.selectRow(0), "snapshot selects wav row");
        waitFor([&] { return controller.acceptedGeneration() >= 1 && controller.selectedPeak() > 0.60; },
            3000, "snapshot analysis finishes");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        c16::require(saveWidgetSnapshot(window.get(), snapshotPath), "snapshot writes window grab");
        controller.close();
        window->close();
        window.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    });
}

int runWorkbenchApp(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QStringList args = app.arguments();
    const int fixtureIndex = args.indexOf("--fixtures");
    const QString fixtures = fixtureIndex >= 0 && fixtureIndex + 1 < args.size()
        ? args[fixtureIndex + 1]
        : QDir::currentPath();
    const int snapshotIndex = args.indexOf("--snapshot");
    const QString snapshot = snapshotIndex >= 0 && snapshotIndex + 1 < args.size()
        ? args[snapshotIndex + 1]
        : QString{};
    if (args.contains("--self-check") || args.contains("--smoke")) {
        return runWorkbenchSelfCheck(fixtures, snapshot);
    }
    if (!snapshot.isEmpty()) {
        return runWorkbenchSnapshot(fixtures, snapshot);
    }
    SessionController controller;
    auto window = std::unique_ptr<QWidget>(createWorkbenchWindow(controller));
    window->resize(960, 640);
    window->show();
    const int code = app.exec();
    controller.close();
    return code;
}

} // namespace workbench
