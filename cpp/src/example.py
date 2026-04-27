import sys
sys.path.append("/home/jetson/project/OrbbecNky/pipeline/temporal-rvl/cpp/build")
from trvl import TrvlEncoder, TrvlDecoder
from pathlib import Path
import cv2,time

def encode_example(path="/home/jetson/project/OrbbecNky/quantize/frame_back",trvlfile = "/home/jetson/project/OrbbecNky/quantize/frame_trvl/data.trvl"):
    path = Path(path)
    encoder = TrvlEncoder(1280, 800, 30, 5, 3, trvlfile)
    frame_count = 0

    for png_file in sorted(path.glob("*.png")):
        print("png file: ",png_file)
        image = cv2.imread(png_file,-1)
        time_now = int(time.time()*1000)
        encoder.encode_trvl(image, time_now)
        frame_count+=1
        if frame_count==15*60:
            break
def decode_example(trvlfile = "/home/jetson/project/OrbbecNky/quantize/frame_trvl/data.trvl"):        
    decoder = TrvlDecoder(trvlfile)
    frame_count = 0

    while True:
        time_now, image =decoder.decode_trvl()
        if not time_now:
            break
        frame_count+=1
        print("frame_count: ",frame_count,image.shape, time_now)

if __name__ == "__main__":
    # encode_example()
    decode_example("/home/jetson/project/OrbbecNky/camera/2025-08-18_15-45-42_bl0_bh0_sw0_aw0_hw0_cg0_ag0_hg0_bw0_bt0/1.rvl")


