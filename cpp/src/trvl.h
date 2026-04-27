#pragma once

#include "rvl.h"
#include "brotli.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <fstream>

//整体约10-30倍压缩

namespace trvl
{
struct Pixel
{
public:
    Pixel() : value(0), invalid_count(0) {}
    short value;
    short invalid_count;
};

short abs_diff(short x, short y)
{
    if (x > y)
        return x - y;
    else
        return y - x;
}

void update_pixel(Pixel& pixel, short raw_value, short change_threshold, int invalidation_threshold) {
    if (pixel.value == 0) {
        if (raw_value > 0)
            pixel.value = raw_value;

        return;
    }


    // Reset the pixel if the depth value indicates the input was invalid two times in a row.
    if (raw_value == 0) {
        ++pixel.invalid_count;
        if (pixel.invalid_count >= invalidation_threshold) {
            pixel.value = 0;
            pixel.invalid_count = 0;
        }
        return;
    }
    pixel.invalid_count = 0;

    // Update pixel value when change is detected.
    if (abs_diff(pixel.value, raw_value) > change_threshold)
        pixel.value = raw_value;
}

class VectorFileManager {
    private:
        std::fstream file;
        std::streampos fileSize;
    public:
        int width, height, keyframe_interval;
    
    public:
        // 构造函数：打开文件
        VectorFileManager(const std::string& filename, short status=0) {
            if(status==0){
                file = std::fstream(filename, std::ios::binary | std::ios::out | std::ios::trunc);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + filename);
                }
            }else{
                file = std::fstream(filename, std::ios::binary | std::ios::in);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + filename);
                }
                // 获取文件的总大小
                std::streampos currentPos = file.tellg();
                file.seekg(0, std::ios::end);  // 移动到文件末尾
                fileSize = file.tellg();
                file.seekg(currentPos);        // 恢复到原来的位置
            }
        }      
    
        // 析构函数：关闭文件
        ~VectorFileManager() {
            if (file.is_open()) {
                file.close();
            }
        }
        
        int save_info(int width_, int height_, int keyframe_interval_){
            width = width_;
            height = height_;
            keyframe_interval = keyframe_interval_;
            file.write(reinterpret_cast<const char*>(&width), sizeof(width));
            file.write(reinterpret_cast<const char*>(&height), sizeof(height));
            file.write(reinterpret_cast<const char*>(&keyframe_interval), sizeof(keyframe_interval));
            return 0;
        }

        int get_info(){            
            file.read(reinterpret_cast<char*>(&width), sizeof(width));
            file.read(reinterpret_cast<char*>(&height), sizeof(height));
            file.read(reinterpret_cast<char*>(&keyframe_interval), sizeof(keyframe_interval));
            return 0;
        }

        // 保存一个vector到文件中  TODO 是否考虑加个结尾标识符号，代表是完整保存了的，异常情况会不会没保存完？
        bool saveVector(const std::vector<char>& vec, const double scale, const int64_t time_now) {
            

            file.write(reinterpret_cast<const char*>(&time_now), sizeof(time_now));
            file.write(reinterpret_cast<const char*>(&scale), sizeof(scale));
            // 记录vector的长度
            size_t size = vec.size();
            file.write(reinterpret_cast<const char*>(&size), sizeof(size));
            // 写入vector数据
            file.write(vec.data(), size);
            return file.good();
        }
    
        // 从文件中读取一个vector
        bool loadVector(std::vector<char>& vec, double& scale, int64_t& time_now) {

            // // 获取文件的当前读取位置
            // std::streampos currentPos = file.tellg();
            // // 计算剩余数据量
            // std::streamsize remainingData = fileSize - currentPos;
            // std::cout << "Remaining data: " << remainingData << " bytes\n";
            // if(remainingData==0) return false;

            file.read(reinterpret_cast<char*>(&time_now), sizeof(time_now));
            
            if (file.eof()) {
                return false; // 文件已结束
            }
            file.read(reinterpret_cast<char*>(&scale), sizeof(scale));

            // 读取vector的长度
            size_t size;
            file.read(reinterpret_cast<char*>(&size), sizeof(size));

            // 读取vector数据
            vec.resize(size);
            file.read(vec.data(), size);
            return true;
        }
    
        // 移动文件指针到文件末尾，方便追加
        void moveToEnd() {
            file.seekp(0, std::ios::end);
        }
    
        // 移动文件指针到文件开头，方便读取
        void moveToStart() {
            file.seekg(0, std::ios::beg);
        }
    };

class TrvlEncoder
{
public:
    TrvlEncoder(int width_, int height_, int keyframe_interval, int change_threshold, int invalid_threshold, const char* filename)
        : pixels_(width_*height_), keyframe_interval_(keyframe_interval), change_threshold_(change_threshold), invalid_threshold_(invalid_threshold), manager(filename)
    {
        frame_size = width_*height_;
        manager.save_info(width_, height_, keyframe_interval);
        
    }

    int encode_trvl(cv::Mat& image, int64_t time_now)
    {
        // bool keyframe = frame_count++ % keyframe_interval_ == 0;
        // cv::Mat downsampledImage;
        
        // cv::resize(image, downsampledImage, cv::Size(), 0.5, 0.5, cv::INTER_LINEAR);
        // double minVal, maxVal;
        // cv::minMaxLoc(image, &minVal, &maxVal);
        // double scale = maxVal / 1000;
        // cv::Mat image_normalize;
        // cv::normalize(image, image_normalize, 0, 1000, cv::NORM_MINMAX, CV_16U);
        
        short* depth_buffer = reinterpret_cast<short*>(image.data);

        // std::vector<char> trvl_frame = encode(depth_buffer,keyframe);

        std::vector<char> trvl_frame =rvl::compress(depth_buffer, frame_size);


        //     // 打印前50个元素
        // int count = 0; // 用于计数
        // for (size_t i = 0; i < trvl_frame.size() && count < 50; ++i) {
        //     std::cout << trvl_frame[i];
        //     ++count;
        // }

        // std::cout << std::endl;
        // std::vector<char> huffman_frame = HuffmanEncode(trvl_frame);
        // std::vector<char> compressed;
        // if (!BrotliCompress(trvl_frame, compressed)) {
        //     printf("errrrror\n");
        // }
        
        manager.saveVector(trvl_frame, 1.0, time_now);
        // manager.saveVector(compressed, scale);

        return 0;
    }

private:
    std::vector<Pixel> pixels_;
    short change_threshold_;
    int invalid_threshold_;
    int keyframe_interval_;
    int frame_size;
    int frame_count = 0;
    VectorFileManager manager;

    std::vector<char> encode(short* depth_buffer, bool keyframe)
    {
        auto frame_size = pixels_.size();
        if (keyframe) {
            for (int i = 0; i < frame_size; ++i) {
                pixels_[i].value = depth_buffer[i];
                // Not sure this is the best way to set invalid_count...
                pixels_[i].invalid_count = depth_buffer[i] == 0 ? 1 : 0;
            }

            return rvl::compress(depth_buffer, frame_size);
        }

        std::vector<short> pixel_diffs(frame_size);
        for (int i = 0; i < frame_size; ++i) {
            pixel_diffs[i] = pixels_[i].value;
            update_pixel(pixels_[i], depth_buffer[i], change_threshold_, invalid_threshold_);
            pixel_diffs[i] = pixels_[i].value - pixel_diffs[i];
        }

        return rvl::compress(pixel_diffs.data(), frame_size);
    }

};

class TrvlDecoder
{
public:
    TrvlDecoder(const char* filename)
        :  manager(filename, 1)
    {   
        manager.get_info();
        height = manager.height;
        width = manager.width;
        frame_size = manager.width*manager.height;
        printf("get info frome file: %d %d %d\n",width,height,manager.keyframe_interval);
        prev_pixel_values_ = std::vector<short>(manager.width*manager.height,0);
    }

    std::pair<int64_t, cv::Mat> decode_trvl(){
        
        double scale;
        int64_t time_now;
        std::vector<char> trvl_frame;
        bool status = manager.loadVector(trvl_frame,scale,time_now);
        if(status){            
            // bool keyframe = frame_count++ % manager.keyframe_interval == 0;
            // auto depth_image = decode(trvl_frame.data(), keyframe);

            // cv::Mat depth_mat(height, width, CV_16UC1, depth_image.data());
            // cv::Mat depth_mat_dequantize;
            // cv::multiply(depth_mat, scale, depth_mat_dequantize);
            // cv::Mat upsampledImage;
            // cv::resize(depth_mat_dequantize, upsampledImage, cv::Size(), 2, 2, cv::INTER_LINEAR);
            // printf("sssssss: %f\n",scale);
            decode_frame = rvl::decompress(trvl_frame.data(), frame_size);
            depth_mat = cv::Mat(height, width, CV_16UC1, decode_frame.data());

            return {time_now, depth_mat};
        }else{
            cv::Mat emptyMat;
            return {0, emptyMat};
        }
        
    }

private:
    std::vector<short> prev_pixel_values_;
    std::vector<short> decode_frame;
    cv::Mat depth_mat;
    VectorFileManager manager;
    int frame_count = 0;
    int width, height, width2,height2;
    int frame_size;

    std::vector<short> decode(char* trvl_frame, bool keyframe)
    {
        int frame_size = prev_pixel_values_.size();
        if (keyframe) {
            prev_pixel_values_ = rvl::decompress(trvl_frame, frame_size);
            return prev_pixel_values_;
        }

        auto pixel_diffs = rvl::decompress(trvl_frame, frame_size);
        for (int i = 0; i < frame_size; ++i)
            prev_pixel_values_[i] += pixel_diffs[i];

        return prev_pixel_values_;
    }
};
}