#include <solution.hpp>
#include <c16/check.hpp>

#include <QImage>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif

namespace {

void check_mapping_lifetime_and_stride()
{
    QImage image(3, 2, QImage::Format_RGBA8888);
    image.fill(QColor(10, 20, 30, 255));
    QVideoFrame frame(image);
    frame.setStartTime(1000);
    frame.setEndTime(2000);

    const auto info = c16_l12::inspect_frame(frame);
    c16::require(info.valid, "QImage-backed QVideoFrame maps");
    c16::require(info.width == 3 && info.height == 2, "frame size is reported");
    c16::require(info.plane_count >= 1, "mapped frame has at least one plane");
    c16::require(info.bytes_per_line0 >= 12, "stride is at least packed RGBA width");
    c16::require(info.mapped_bytes0 >= info.bytes_per_line0 * info.height, "mapped bytes cover stride times height");
    c16::require(info.start_time == 1000 && info.end_time == 2000, "frame timestamps are preserved");
    c16::require(!frame.isMapped(), "inspect_frame unmaps before returning");
}

void check_invalid_and_yuv()
{
    QVideoFrame invalid;
    c16::require(!c16_l12::inspect_frame(invalid).valid, "invalid frame is reported without mapping");
    const auto black = c16_l12::limited_yuv_to_rgb(16, 128, 128);
    c16::require(black.r <= 1 && black.g <= 1 && black.b <= 1, "limited-range black converts near zero");
    const auto white = c16_l12::limited_yuv_to_rgb(235, 128, 128);
    c16::require(white.r >= 254 && white.g >= 254 && white.b >= 254, "limited-range white converts near full");
    const auto redish = c16_l12::limited_yuv_to_rgb(81, 90, 240);
    c16::require(redish.r > redish.g && redish.r > redish.b, "YUV chroma affects RGB channels");
}

void check_multiplane_and_padding_observation()
{
    QVideoFrame yuv(QVideoFrameFormat(QSize(4, 4), QVideoFrameFormat::Format_YUV420P));
    const auto yuv_info = c16_l12::inspect_frame(yuv);
    c16::require(yuv_info.valid, "allocated YUV420P frame maps");
    c16::require(yuv_info.plane_count >= 3, "YUV420P exposes multiple planes");

    QImage padded(1, 2, QImage::Format_RGB888);
    padded.fill(QColor(1, 2, 3));
    QVideoFrame padded_frame(padded);
    const auto padded_info = c16_l12::inspect_frame(padded_frame);
    c16::require(padded_info.valid, "RGB888 padded frame maps");
    c16::require(padded_info.bytes_per_line0 > padded_info.width * 3, "stride preserves row padding beyond visible RGB bytes");
}

} // namespace

int main()
{
    return c16::run([] {
        check_mapping_lifetime_and_stride();
        check_invalid_and_yuv();
        check_multiplane_and_padding_observation();
    });
}
