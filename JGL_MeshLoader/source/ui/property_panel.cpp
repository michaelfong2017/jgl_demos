#include "pch.h"
#include "property_panel.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace std;

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


namespace nui
{
    bool decodeFrame(AVFormatContext* formatContext, int videoStreamIndex, AVFrame*& outputFrame, int targetFrameCount)
    {
        // Get the codec and codec context
        AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
        AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecContext, codecParameters);

        // Open the codec
        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            std::cerr << "Could not open codec.\n";
            return false;
        }

        AVFrame* frame = av_frame_alloc();
        AVPacket packet;
        int frameCount = 0;

        // SwsContext *img_convert_ctx = sws_getContext(codecContext->width,
        //             codecContext->height, codecContext->pix_fmt, codecContext->width,
        //             codecContext->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL,
        //             NULL, NULL);

        // Read the frames
        while (av_read_frame(formatContext, &packet) >= 0)
        {
            if (packet.stream_index == videoStreamIndex)
            {
                // Decode the video frame
                if (avcodec_send_packet(codecContext, &packet) < 0)
                {
                    std::cerr << "Error sending packet for decoding.\n";
                    return false;
                }

                int ret = avcodec_receive_frame(codecContext, frame);
                if (ret == 0)
                {
                    // Stop decoding when the third frame is found
                    if (frameCount++ == targetFrameCount)
                    {
                        // outputFrame = frame;

                        // Convert ffmpeg frame timestamp to real frame number.
                        // int64_t numberFrame = (double)((int64_t)pts -
                        // 	pFormatCtx->streams[videoStreamIndex]->start_time) *
                        // 	videoBaseTime * videoFramePerSecond;

                        // Get RGBA Frame
                        AVFrame* rgbaFrame = NULL;
                        int width = codecContext->width;
                        int height = codecContext->height;
                        int bufferImgSize = avpicture_get_size(AV_PIX_FMT_BGR24, width, height);
                        rgbaFrame = av_frame_alloc();
                        uint8_t* buffer = (uint8_t*)av_mallocz(bufferImgSize);
                        if (rgbaFrame)
                        {
                            avpicture_fill((AVPicture*)rgbaFrame, buffer, AV_PIX_FMT_BGR24, width, height);
                            rgbaFrame->width = width;
                            rgbaFrame->height = height;
                            // rgbaFrame->data[0] = buffer;

                            SwsContext* pImgConvertCtx = sws_getContext(codecContext->width, codecContext->height,
                                codecContext->pix_fmt,
                                codecContext->width, codecContext->height,
                                AV_PIX_FMT_BGR24,
                                SWS_BICUBIC, NULL, NULL, NULL);

                            sws_scale(pImgConvertCtx, frame->data, frame->linesize,
                                0, height, rgbaFrame->data, rgbaFrame->linesize);
                        }

                        outputFrame = (AVFrame*)rgbaFrame;
                        break;
                    }
                }
            }
            av_packet_unref(&packet);
        }

        // Clean up
        av_packet_unref(&packet);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);

        return frameCount == targetFrameCount + 1;
    }

    GLuint createTextureFromFrame(AVFrame* frame)
    {
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // // glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

        // GLenum format = frame->format == AV_PIX_FMT_RGB24 ? GL_RGB : GL_RGBA;
        // glTexImage2D(GL_TEXTURE_2D, 0, format, frame->width, frame->height, 0, format, GL_UNSIGNED_BYTE, frame->data[0]);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload the frame data to the texture object
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);

        return textureId;
    }

  void Property_Panel::render(nui::SceneView* scene_view)
  {
    auto mesh = scene_view->get_mesh();

    ImGui::Begin("Properties");
    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
      
      if (ImGui::Button("Open..."))
      {
        mFileDialog.Open();
      }
      ImGui::SameLine(0, 5.0f);
      ImGui::Text(mCurrentFile.c_str());
    }

    if (ImGui::CollapsingHeader("Material") && mesh)
    {
      ImGui::ColorPicker3("Color", (float*)&mesh->mColor, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
      ImGui::SliderFloat("Roughness", &mesh->mRoughness, 0.0f, 1.0f);
      ImGui::SliderFloat("Metallic", &mesh->mMetallic, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Light"))
    {

      ImGui::Separator();
      ImGui::Text("Position");
      ImGui::Separator();
      nimgui::draw_vec3_widget("Position", scene_view->get_light()->mPosition, 80.0f);
    }

    ImGui::End();

    mFileDialog.Display();
    if (mFileDialog.HasSelected())
    {
      //auto file_path = mFileDialog.GetSelected().string();
      //mCurrentFile = file_path.substr(file_path.find_last_of("/\\") + 1);

      //mMeshLoadCallback(file_path);

      //mFileDialog.ClearSelected();
        cout << "Selected filename is " << mFileDialog.GetSelected().string() << endl;

        //cout << "Current path is " << filesystem::current_path() << endl; // (1)
        //system("chmod +x /echocardiography-ui/packages/DICOMTestExe/DICOMTestExe");
        //string command = "yes | /echocardiography-ui/packages/DICOMTestExe/DICOMTestExe " + mFileDialog.GetSelected().string() + " A2C";
        //system(command.c_str());
        std::string current_path = std::filesystem::current_path().string();
        std::string command = "cd ..\\3DEchocardiography\\post && ..\\miniconda3\\condabin\\conda activate base && yes | python DICOMTestExe.py " + mFileDialog.GetSelected().string() + " A2C && cd " + current_path;
        if (ends_with(current_path, "x64\\Debug")) {
            command = "cd .. && " + command;
        }
        std::cout << "command: " << command << std::endl;
        std::system(command.c_str());

        mFileDialog.ClearSelected();

        showVideo = true;
    }

    if (showVideo)
    {
        // Read mp4 file
        // Initialize network components (if needed)
        avformat_network_init();

        // Open the input file
        AVFormatContext* formatContext = nullptr;
        std::string inputFilename = "..\\3DEchocardiography\\data\\dcm\\dicomresults\\A2C\\mp4s\\PWHOR190734217S_12Oct2021_CX03WQDU_3DQ.mp4";
        std::string current_path = std::filesystem::current_path().string();
        if (ends_with(current_path, "x64\\Debug")) {
            inputFilename = "..\\" + inputFilename;
        }
        if (avformat_open_input(&formatContext, inputFilename.c_str(), nullptr, nullptr) != 0)
        {
            std::cerr << "Could not open input file.\n";
            return;
        }

        // Find the first video stream
        int videoStreamIndex = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
        {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1)
        {
            std::cerr << "No video stream found.\n";
            return;
        }

        int64_t nb_frames = formatContext->streams[videoStreamIndex]->nb_frames;
        cout << "nb_frames: " << nb_frames << endl;
        int64_t duration = formatContext->streams[videoStreamIndex]->duration;
        double durationInSeconds = (double)duration * av_q2d(formatContext->streams[videoStreamIndex]->time_base);
        cout << "durationInSeconds: " << durationInSeconds << endl;
        double frameRate = av_q2d(formatContext->streams[videoStreamIndex]->r_frame_rate);
        cout << "frameRate: " << frameRate << endl;

        // Decode frame
        ImGui::Begin("Video Player");
        ImGui::SetWindowSize(ImVec2(720, 480));
        currentIoFrameNumber = currentIoFrameNumber + frameRate / ImGui::GetIO().Framerate;
        if (currentIoFrameNumber > 90)
        {
            currentIoFrameNumber -= 90;
        }
        if (currentIoFrameNumber < nb_frames)
        {
            AVFrame* targetFrame = nullptr;
            if (!decodeFrame(formatContext, videoStreamIndex, targetFrame, currentIoFrameNumber))
            {
                std::cerr << "Could not decode the target frame.\n";
            }
            else {
                GLuint textureId = createTextureFromFrame(targetFrame);
                av_frame_free(&targetFrame);

                ImVec2 window_size = ImGui::GetWindowSize();
                ImGui::Image((void*)(intptr_t)textureId, window_size);
            }
        }
        ImGui::End();
    }

  }
}
