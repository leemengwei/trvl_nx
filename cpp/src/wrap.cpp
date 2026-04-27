#include "trvl.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

PYBIND11_MODULE(trvl, m) {
    py::class_<trvl::TrvlEncoder>(m, "TrvlEncoder")
        .def(py::init<int, int, int, int, int, const char*>(),
             py::arg("width"),
             py::arg("height"),
             py::arg("keyframe_interval"),
             py::arg("change_threshold"),
             py::arg("invalid_threshold"),
             py::arg("filename"))
        .def("encode_trvl", [](trvl::TrvlEncoder& self, py::array_t<uint16_t>& image, int64_t time_now) {
            // 将 numpy 数组转换为 OpenCV Mat
            py::buffer_info buf = image.request();
            cv::Mat mat(buf.shape[0], buf.shape[1], CV_16UC1, buf.ptr);

            // 调用 encode_trvl 方法
            return self.encode_trvl(mat, time_now);
        }, py::arg("image"),py::arg("time_now"));


    py::class_<trvl::TrvlDecoder>(m, "TrvlDecoder")
        .def(py::init<const char*>(), py::arg("filename"))
        .def("decode_trvl", [](trvl::TrvlDecoder& self) {
            // 调用 decode_trvl 方法
            int64_t time_now = 0;
            cv::Mat depth_mat;
            std::tie(time_now, depth_mat) = self.decode_trvl();
            // 将返回的 cv::Mat 转换为 numpy 数组
            if (time_now) { // 如果解码成功
                py::array_t<uint16_t> numpy_array({depth_mat.rows, depth_mat.cols}, reinterpret_cast<uint16_t*>(depth_mat.data));
                // // 创建一个 numpy 数组，并显式复制 depth_mat 的数据
                // py::array_t<uint16_t> numpy_array({depth_mat.rows, depth_mat.cols});
                // auto numpy_ptr = numpy_array.mutable_data();
                // std::memcpy(numpy_ptr, depth_mat.data, depth_mat.total() * sizeof(uint16_t));
                return py::make_tuple(time_now, numpy_array);
            } else { // 如果解码失败
                return py::make_tuple(0, py::array_t<uint16_t>());
            }
        });
}
