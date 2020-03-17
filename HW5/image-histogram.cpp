#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
// why this size?? https://github.com/smistad/OpenCL-Getting-Started/blob/master/main.c
#define MAX_SOURCE_SIZE (0x100000)
using namespace std;
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

void check(cl_int err, const char *msg) {
    if (err != CL_SUCCESS) {
        cerr << msg << endl;
        // std::cerr << getErrorString(err) << std::endl;
        exit(-1);
    }
}
Image *readbmp(const char *filename) {
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32) {
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
    for (int i = 0; i < ret->size; i++) {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img) {

    uint8_t header[54] = {
        0x42, // identity : B
        0x4d, // identity : M
        0, 0, 0, 0, // file size
        0, 0, // reserved1
        0, 0, // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0, // bmp width
        0, 0, 0, 0, // bmp height
        1, 0, // planes
        32, 0, // bit per pixel
        0, 0, 0, 0, // compression
        0, 0, 0, 0, // data size
        0, 0, 0, 0, // h resolution
        0, 0, 0, 0, // v resolution
        0, 0, 0, 0, // used colors
        0, 0, 0, 0 // important colors
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

void histogram(Image *img, uint32_t R[256], uint32_t G[256], uint32_t B[256]) {
    std::fill(R, R + 256, 0);
    std::fill(G, G + 256, 0);
    std::fill(B, B + 256, 0);

    for (int i = 0; i < img->size; i++) {
        RGB &pixel = img->data[i];
        R[pixel.R]++;
        G[pixel.G]++;
        B[pixel.B]++;
    }
}

int main(int argc, char *argv[]) {
    FILE *fp;
    char *source_str;
    size_t source_size;
    fp = fopen("kernel.cl", "r");
    if (!fp) {
        cout << "source file open failed!" << endl;
        exit(1);
    }
    source_str = (char *)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);

    char *filename;
    if (argc >= 2) {
        int many_img = argc - 1;
        for (int i = 0; i < many_img; i++) {
            filename = argv[i + 1];
            Image *img = readbmp(filename);

            std::cout << img->weight << ":" << img->height << "\n";

            uint32_t R[256];
            uint32_t G[256];
            uint32_t B[256];
            std::fill(R, R + 256, 0);
            std::fill(G, G + 256, 0);
            std::fill(B, B + 256, 0);

#ifdef SERIAL
            histogram(img, R, G, B);
#else

            RGB *pixels = img->data;
            cl_int err;
            cl_uint num;
            cl_device_id device_id;
            // size_t bytes = sizeof(RGB) * 256;
            err = clGetPlatformIDs(0, 0, &num);
            if (err != CL_SUCCESS) {
                std::cerr << "Unable to get platforms\n";
                return 0;
            }

            std::vector<cl_platform_id> platforms(num);

            // get platform id
            err = clGetPlatformIDs(num, &platforms[0], &num);
            if (err != CL_SUCCESS) {
                std::cerr << "Unable to get platform ID\n";
                return 0;
            }

            //get device id
            err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num, &device_id, NULL);
            if (err != CL_SUCCESS) {
                std::cerr << "Unable to get device ID\n";
                return 0;
            }

            // create context
            //cl_context context = clCreateContextFromType(0, CL_DEVICE_TYPE_CPU, NULL, NULL, &err);
            cl_context context = clCreateContext(0, num, &device_id, NULL, NULL, &err);
            check(err, "Unable to create context");

            // create command queue
            cl_command_queue queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
            check(err, "Unable to create command queue");

            // create buffer
            cl_mem buffer_img, buffer_histo, buffer_r, buffer_g, buffer_b;
            //buffer_img = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(RGB) * img->size, NULL, &err);
            buffer_img = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(RGB) * img->size, NULL, &err);
            check(err, "Unable to create buffer for buffer_img");
            buffer_r = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &err);
            check(err, "Unable to create buffer");
            buffer_g = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &err);
            check(err, "Unable to create buffer");
            buffer_b = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * 256, NULL, &err);
            check(err, "Unable to create buffer");

            // assign buffer
            err = clEnqueueWriteBuffer(queue, buffer_img, CL_TRUE, 0, sizeof(RGB) * img->size, pixels, 0, 0, 0);
            err = clEnqueueWriteBuffer(queue, buffer_r, CL_TRUE, 0, sizeof(uint32_t) * 256, R, 0, NULL, NULL);
            err = clEnqueueWriteBuffer(queue, buffer_g, CL_TRUE, 0, sizeof(uint32_t) * 256, G, 0, NULL, NULL);
            err = clEnqueueWriteBuffer(queue, buffer_b, CL_TRUE, 0, sizeof(uint32_t) * 256, B, 0, NULL, NULL);
            check(err, "Error in buffer_img memory transfer");

            // buffer_histo = clCreateBuffer(context, CL_MEM_WRITE_ONLY, bytes, NULL, &err);

            //create program
            cl_program program = clCreateProgramWithSource(context, 1,
                (const char **)&source_str, NULL, &err);
            check(err, "Error in create program");

            // Build the program executable
            err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
            check(err, "Error in building program");

            // Create the compute kernel in the program we wish to run
            cl_kernel kernel = clCreateKernel(program, "kernel_histogram", &err);
            check(err, "Error in create kernel");

            // set arguments
            err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_img);
            err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_r);
            err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_g);
            err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &buffer_b);
            check(err, "error in clSetKernelArg of buffer");

            // err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_histo);
            // err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_a);
            // check(err, "error in clSetKernelArg of buffer_histo");

            size_t work_size = (size_t)img->size;
            // cout << img->size << endl;
            // cout << work_size << endl;
            err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_size, NULL, 0, NULL, NULL);
            check(err, "Error in EnqueueNDrange ");

            err = clEnqueueReadBuffer(queue, buffer_r, CL_TRUE, 0, sizeof(uint32_t) * 256, R, 0, NULL, NULL);
            err = clEnqueueReadBuffer(queue, buffer_g, CL_TRUE, 0, sizeof(uint32_t) * 256, G, 0, NULL, NULL);
            err = clEnqueueReadBuffer(queue, buffer_b, CL_TRUE, 0, sizeof(uint32_t) * 256, B, 0, NULL, NULL);
            check(err, "Error in Enqueue read buffer");
            err = clFlush(queue);
            check(err, "Error in clFinish");
            err = clFinish(queue);
            check(err, "Error in clFlush");

            clReleaseContext(context);
            clReleaseMemObject(buffer_img);
            clReleaseMemObject(buffer_r);
            clReleaseMemObject(buffer_g);
            clReleaseMemObject(buffer_b);

#endif

            int max = 0;
            for (int i = 0; i < 256; i++) {
                // if (i < 20)
                cout << R[i] << endl;
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];

            for (int i = 0; i < ret->height; i++) {
                for (int j = 0; j < 256; j++) {
                    if (R[j] * 256 / max > i)
                        ret->data[256 * i + j].R = 255;
                    if (G[j] * 256 / max > i)
                        ret->data[256 * i + j].G = 255;
                    if (B[j] * 256 / max > i)
                        ret->data[256 * i + j].B = 255;
                }
            }

            std::string newfile = "hist_" + std::string(filename);
            writebmp(newfile.c_str(), ret);
        }
    } else {
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}
