#pragma once

#include <QAbstractListModel>
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioOutput>
#include <QJsonArray>
#include <QJsonObject>
#include <QMediaPlayer>
#include <QPointer>
#include <QSaveFile>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVideoWidget>
#include <QWidget>

#include <atomic>
#include <memory>
#include <vector>

namespace workbench {

struct MediaItem {
    QString id;
    QString path;
    QString displayName;
    QString error;
    qint64 durationMs = 0;
    double peak = 0.0;
    QVector<float> waveform;
};

struct AnalysisResult {
    int generation = 0;
    QString id;
    QString path;
    QString error;
    qint64 durationMs = 0;
    double peak = 0.0;
    QVector<float> waveform;
    int audioBuffers = 0;
    int videoFrames = 0;
};

class MediaListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        PathRole,
        DisplayNameRole,
        ErrorRole,
        DurationRole,
        PeakRole,
        SelectedRole,
        RowRole
    };

    explicit MediaListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int addFile(const QString& path);
    Q_INVOKABLE void setFilter(const QString& text);
    Q_INVOKABLE QString filter() const;
    Q_INVOKABLE bool selectRow(int row);
    Q_INVOKABLE QString selectedId() const;
    Q_INVOKABLE QString selectedPath() const;
    Q_INVOKABLE QString selectedName() const;
    Q_INVOKABLE void applyAnalysis(const AnalysisResult& result);
    Q_INVOKABLE QJsonObject toJson() const;
    Q_INVOKABLE bool restore(const QJsonObject& object);
    bool prepareRestore(const QJsonObject& object, QVector<MediaItem>& items, QVector<int>& visible,
        QString& selected, QString& filter, QString* error) const;

    const MediaItem* itemById(const QString& id) const;

private:
    static QString makeId(const QString& path);
    void rebuildFilter();
    bool matches(const MediaItem& item) const;

    QVector<MediaItem> items_;
    QVector<int> visible_;
    QString filter_;
    QString selectedId_;
};

struct SessionSnapshot {
    QVector<MediaItem> items;
    QVector<int> visible;
    QString selected;
    QString filter;
    QJsonArray markers;
};

class AnalysisWorker final : public QObject {
    Q_OBJECT
public:
    AnalysisResult analyze(QString id, QString path, int generation, std::shared_ptr<std::atomic_bool> cancel);
};

class SessionController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(MediaListModel* mediaModel READ model CONSTANT)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
public:
    explicit SessionController(QObject* parent = nullptr);
    ~SessionController() override;

    MediaListModel* model();
    QMediaPlayer* player();
    QVideoSink* videoSink() const;

    Q_INVOKABLE bool addMedia(const QString& path);
    Q_INVOKABLE bool selectRow(int row);
    Q_INVOKABLE void setFilter(const QString& text);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void seek(int ms);
    Q_INVOKABLE bool addMarker(qint64 ms, const QString& label);
    Q_INVOKABLE bool saveSession(const QString& path);
    Q_INVOKABLE bool restoreSession(const QString& path);
    Q_INVOKABLE QString selectedName() const;
    Q_INVOKABLE QString errorText() const;
    Q_INVOKABLE bool playing() const;
    Q_INVOKABLE int acceptedGeneration() const;
    Q_INVOKABLE int decodedAudioBuffers() const;
    Q_INVOKABLE int decodedVideoFrames() const;
    Q_INVOKABLE double selectedPeak() const;
    Q_INVOKABLE qint64 selectedDurationMs() const;
    Q_INVOKABLE QString markerSummary() const;

    void attachVideo(QVideoWidget* widget);
    void close();

signals:
    void stateChanged();

private:
    void startAnalysis();
    void acceptAnalysis(const AnalysisResult& result);
    void setError(QString error);
    bool parseSessionFile(const QString& path, SessionSnapshot& snapshot, QString* error) const;
    void applySessionSnapshot(const SessionSnapshot& snapshot);

    MediaListModel model_;
    QThread workerThread_;
    AnalysisWorker* worker_ = nullptr;
    QMediaPlayer player_;
    QAudioOutput audio_;
    QAudioBufferOutput audioTap_;
    QPointer<QVideoWidget> videoWidget_;
    std::shared_ptr<std::atomic_bool> cancel_ = std::make_shared<std::atomic_bool>(false);
    std::atomic<int> generation_{0};
    std::atomic_bool closing_{false};
    int acceptedGeneration_ = 0;
    int audioBuffers_ = 0;
    int videoFrames_ = 0;
    QString error_;
    QJsonArray markers_;
};

QWidget* createWorkbenchWindow(SessionController& controller);
int runWorkbenchApp(int argc, char** argv);
int runWorkbenchSelfCheck(const QString& fixtureDir, const QString& snapshotPath);

} // namespace workbench
