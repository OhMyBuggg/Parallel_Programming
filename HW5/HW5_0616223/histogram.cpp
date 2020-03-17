#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_SOURCE_SIZE (0x100000)
#define OPENCL_CHECK(val) check_opencl_call((val), __FILE__, __LINE__)

using namespace std;

void check_opencl_call(cl_int val, const char *const file, int const line)
{   
    if (val != CL_SUCCESS) {
            //cout << val << endl;
        printf("OpenCL error at %s:%d\n", file, line);
    }
}

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

void histogram(Image *img,uint32_t R[256],uint32_t G[256],uint32_t B[256],char *source_str){
    std::fill(R, R+256, 0);
    std::fill(G, G+256, 0);
    std::fill(B, B+256, 0);

    RGB *pixel = img->data;

    //cout << "here is historam" << endl;

    cl_int ret;
    cl_platform_id* platform_id;
    cl_uint ret_num_platforms;
    cl_device_id *device_id;
    cl_uint ret_num_devices;

    ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
    OPENCL_CHECK(ret);

    platform_id = (cl_platform_id *)malloc(sizeof(cl_platform_id) * ret_num_platforms);
    ret = clGetPlatformIDs(ret_num_platforms, platform_id, NULL);
    OPENCL_CHECK(ret);

    //cout << "get all platform done" << endl;

    ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_ALL, 0, NULL, &ret_num_devices);
    OPENCL_CHECK(ret);
    device_id = (cl_device_id *)malloc(sizeof(cl_device_id) * ret_num_devices);
    ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_ALL, ret_num_devices, device_id, NULL);
    OPENCL_CHECK(ret);

    //cout << "get all device done" << endl;

    cl_context context = clCreateContext(NULL, ret_num_devices, device_id, NULL, NULL, &ret);
    OPENCL_CHECK(ret);

    //cout << "create context deon" << endl;

    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
    OPENCL_CHECK(ret);

    //cout << "create command_queue done" << endl;

    cl_mem bufferImg;
    cl_mem bufferR;
    cl_mem bufferG;
    cl_mem bufferB;

    bufferImg = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(RGB) * img->size, NULL, &ret);
    OPENCL_CHECK(ret);
    bufferR = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &ret);
    OPENCL_CHECK(ret);
    bufferG = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &ret);
    OPENCL_CHECK(ret);
    bufferB = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &ret);
    OPENCL_CHECK(ret);

    //cout << "cl_mem malloc done" << endl;

    // //cout << "======show image======" << endl;
    // for (int i = 0; i < 5; i++){
    //     RGB &pixel = img->data[i];
    //     //cout << "R: " << pixel.R << " ";
    //     R[pixel.R]++;
    //     //cout << "G: " << pixel.G << " ";
    //     //cout << "B: " << pixel.B << endl;
    //     //cout << "R[pixel.R]: " << R[pixel.R] << endl;
    // }
    // //cout << endl;

    ret = clEnqueueWriteBuffer(command_queue, bufferImg, CL_TRUE, 0, sizeof(RGB) * img->size, pixel, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, bufferR, CL_TRUE, 0, sizeof(uint32_t) * 256, R, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, bufferG, CL_TRUE, 0, sizeof(uint32_t) * 256, G, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(command_queue, bufferB, CL_TRUE, 0, sizeof(uint32_t) * 256, B, 0, NULL, NULL);

    //cout << "cl_mem copy done" << endl;

    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, NULL, &ret);
    OPENCL_CHECK(ret);

    //cout << "create program done" << endl;

    ret = clBuildProgram(program, ret_num_devices, device_id, NULL, NULL, NULL);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
    // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        printf("%s\n", log);
    }
    OPENCL_CHECK(ret);

    //cout << "build program done" << endl;

    cl_kernel kernel = clCreateKernel(program, "histogram", &ret);
    OPENCL_CHECK(ret);

    //cout << "create kernel done" << endl;

    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufferImg);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufferR);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferG);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), &bufferB);

    //cout << "set arg done" << endl;

    //size_t localWorkSize[2] = {8,8} ;
    size_t globalWorkSize[1];
    globalWorkSize[0] = img->weight * img->height;

    //cout << "gpu start runnung" << endl;

    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    OPENCL_CHECK(ret);


    //cout << "gpu done" << endl;

    clEnqueueReadBuffer(command_queue, bufferR, CL_TRUE, 0, sizeof(uint32_t) * 256, R, 0, NULL, NULL);
    OPENCL_CHECK(ret);
    clEnqueueReadBuffer(command_queue, bufferG, CL_TRUE, 0, sizeof(uint32_t) * 256, G, 0, NULL, NULL);
    OPENCL_CHECK(ret);
    clEnqueueReadBuffer(command_queue, bufferB, CL_TRUE, 0, sizeof(uint32_t) * 256, B, 0, NULL, NULL);
    OPENCL_CHECK(ret);

    //cout << "copy from gpu done" << endl;

    ret = clFlush(command_queue);
    OPENCL_CHECK(ret);
    ret = clFinish(command_queue);
    OPENCL_CHECK(ret);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(bufferImg);
    ret = clReleaseMemObject(bufferR);
    ret = clReleaseMemObject(bufferG);
    ret = clReleaseMemObject(bufferB);
    ret = clReleaseCommandQueue(command_queue);

    free(device_id);
    free(platform_id);
}

int main(int argc, char *argv[])
{
    char *filename;
    if (argc >= 2)
    {
        int many_img = argc - 1;
        FILE *fp;
        char *source_str;
        size_t source_size;
        fp = fopen("histogram.cl", "r");
        if(!fp){
            exit(1);
        }
        source_str = (char *)malloc(MAX_SOURCE_SIZE);
        source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);

        //cout << "source_str: ";
        //cout << source_str;
        //cout << endl;
        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            Image *img = readbmp(filename);

            std::cout << img->weight << ":" << img->height << "\n";

            uint32_t R[256];
            uint32_t G[256];
            uint32_t B[256];

            //cout << "try to histogrm" << endl;
            histogram(img,R,G,B,source_str);
            //cout << "histogram done" << endl;

            int max = 0;
            for(int i=0;i<256;i++){
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }

            //cout << "max: " << max << endl;

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];

            //cout << "set ret" << endl;

            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    ////cout << "j: " << j << endl;
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
                }
            }

            //cout << "data copy done" << endl;

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);
            //cout << "output done" << endl;
        }
        free(source_str);
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }

    return 0;
}