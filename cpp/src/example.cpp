#include <filesystem>
#include "trvl.h"
#include <ios>
#include <chrono>

using namespace std;


std::vector<std::string> getFileNames(const std::string& directoryPath) {
    std::vector<std::string> fileNames;

    // 遍历目录
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {  // 只处理普通文件
            fileNames.push_back(entry.path().filename().string());
        }
    }
    std::sort(fileNames.begin(), fileNames.end());

    return fileNames;
}

// 计算误差并生成误差分布图
void calculateAndPlotError(const cv::Mat& original, const cv::Mat& decoded, const std::string& output_path) {
    // 确保图像大小和类型一致
    CV_Assert(original.size() == decoded.size() && original.type() == decoded.type());

    // 计算误差
    cv::Mat error;
    cv::absdiff(original, decoded, error);

    // 将误差图像转换为灰度图（如果是多通道图像）
    if (error.channels() > 1) {
        cv::cvtColor(error, error, cv::COLOR_BGR2GRAY);
    }

    // 计算误差的统计信息
    double minVal, maxVal;
    cv::minMaxLoc(error, &minVal, &maxVal);
    std::cout << "Min error: " << minVal << ", Max error: " << maxVal << std::endl;

    cv::Mat error_normalize;
    cv::normalize(error, error_normalize, 0, 255, cv::NORM_MINMAX, CV_8U);
    
    cv::Mat colored;
    cv::applyColorMap(error_normalize, colored, cv::COLORMAP_BONE);
    cv::imwrite(output_path, colored);
    std::cout << "Error distribution plot saved to: " << output_path << std::endl;
}

void run_encode(){
    const std::vector<std::string> PngFiles = getFileNames("/home/jetson/project/OrbbecNky/quantize/frame");
    int CHANGE_THRESHOLD = 3; //可以理解为容忍多大误差  目前测试是5cm误差151 3cm 185 2cm 212
    int INVALIDATION_THRESHOLD = 3;
    int keyframe_interval = 30;
    int WIDTH = 1280;
    int HEIGHT = 800;
    int frame_size = WIDTH*HEIGHT;
    const char* trvlfile = "/home/jetson/project/OrbbecNky/quantize/frame_trvl/data.rvl";

    trvl::TrvlEncoder encoder(WIDTH, HEIGHT, keyframe_interval, CHANGE_THRESHOLD, INVALIDATION_THRESHOLD, trvlfile);
    // trvl::Decoder decoder("/home/jetson/project/OrbbecNky/quantize/frame_trvl/data.trvl");

    std::vector<short> depth_buffer(frame_size);
    int frame_count = 0;
    
    for(auto png_path : PngFiles){
        cv::Mat image = cv::imread(("/home/jetson/project/OrbbecNky/quantize/frame/"+png_path).c_str(), cv::IMREAD_UNCHANGED);

        printf("png_path: %s\n",png_path.c_str());
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        
        auto trvl_frame = encoder.encode_trvl(image, milliseconds);
        
        if (++frame_count==15*60) break;

        // auto depth_image = decoder.decode(trvl_frame.data(), keyframe);

    }

}

void run_decode(){

    const char* trvlfile = "/home/jetson/project/OrbbecNky/quantize/frame_trvl/data.rvl";
    // const char* trvlfile = "/home/jetson/project/OrbbecNky/0.rtvl";
    trvl::TrvlDecoder decoder(trvlfile);
    int64_t time_now = 1;
    cv::Mat depth_mat;
    int frame_count = 0;
    std::string path_png_base = "/home/jetson/project/OrbbecNky/quantize/";
    while (time_now)
    {       
        std::ostringstream filename_stream;
        std::string filename = "frame_0_";        
        filename_stream << filename << std::setw(8) << std::setfill('0') << frame_count << ".png";

        std::tie(time_now, depth_mat) = decoder.decode_trvl();
        // auto[depth_mat,status] = decoder.decode_trvl();
        if(!time_now) break;
        // // 升采样
        // cv::Mat upsampledImage;
        // cv::resize(depth_mat, upsampledImage, cv::Size(1280, 800), 0, 0, cv::INTER_LINEAR);

        cv::imwrite((path_png_base+"frame_trvl/"+filename_stream.str()).c_str(), depth_mat);
        // 计算并绘制误差图

        cv::Mat image = cv::imread((path_png_base+"frame/"+filename_stream.str()).c_str(), cv::IMREAD_UNCHANGED);
        calculateAndPlotError(image, depth_mat, (path_png_base+"frame_trvl_error/"+filename_stream.str()).c_str());

        // cv::imshow("Depthraw", image);
        // cv::imshow("Depth", depth_mat);
        // cv::waitKey(1);
        printf("count: %d %lld\n", frame_count++, time_now);
        
        // cv::imshow("Depth", depth_mat);
        // cv::waitKey(1);
        
    }
    cv::destroyAllWindows();
}

int main()
{   
    run_encode();
    run_decode();

    return 0;
}