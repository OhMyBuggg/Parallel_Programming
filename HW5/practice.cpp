#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>

#define MAX_SOURCE_SIZE (0x100000)
#define OPENCL_CHECK(val) check_opencl_call((val), __FILE__, __LINE__)

using namespace std;

const char* programSource =
"__kernel                                            \n"
"void vecadd(__global int *A,                        \n"
"            __global int *B,                        \n"
"            __global int *C)                        \n"
"{                                                   \n"
"                                                    \n"
"   // Get the work-item’s unique ID                 \n"
"   int idx = get_global_id(0);                      \n"
"                                                    \n"
"   // Add the corresponding locations of            \n"
"   // 'A' and 'B', and store the result in 'C'.     \n"
"   C[idx] = A[idx] + B[idx];                        \n"
"}                                                   \n"
;

void check_opencl_call(cl_int val, const char *const file, int const line)
{
    if (val != CL_SUCCESS) {
    		cout << val << endl;
        printf("OpenCL error at %s:%d\n", file, line);
    }
}

int main(){
	srand( time(NULL) );
	int len;
	int min = 1;
	int max = 100;

	FILE *fp;
	const char *str;
	char *source_str;
	size_t source_size;
	fp = fopen("add.cl", "r");
	if(!fp){
		exit(1);
	}
	source_str = (char *)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
	str = source_str;

	cout << "programSource: ";
	cout << programSource;
	cout << endl;

	cout << "str: ";
	cout << str;
	cout << endl;

	cin >> len;
	int *a = (int *)malloc(sizeof(int) * len);
	int *b = (int *)malloc(sizeof(int) * len);
	for(int i = 0; i < len; i++){
		a[i] = rand() % (max - min + 1) + min;
		b[i] = rand() % (max - min + 1) + min;
	}

	cl_int ret;
	cl_platform_id* platform_id;
	cl_uint ret_num_platforms;
	cl_device_id* device_id;
	cl_uint ret_num_devices;

	ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);
	OPENCL_CHECK(ret);

	cout << "ret_num_platforms: " << ret_num_platforms << endl;
	platform_id = (cl_platform_id*)malloc(sizeof(cl_platform_id) * ret_num_platforms);
	ret = clGetPlatformIDs(ret_num_platforms, platform_id, NULL);
	OPENCL_CHECK(ret);

	for(int i = 0; i < ret_num_platforms; i++){
		cout << "i: " << i << endl;
		ret = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_GPU, 0, NULL, &ret_num_devices);
		OPENCL_CHECK(ret);
		device_id = (cl_device_id*)malloc(sizeof(cl_device_id) * ret_num_devices);

		ret = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_GPU, ret_num_devices, device_id, NULL);
		OPENCL_CHECK(ret);
		cout << "ret_num_devices: " << ret_num_devices << endl;

		cl_context context = clCreateContext(NULL, ret_num_devices, device_id, NULL, NULL, &ret);
		OPENCL_CHECK(ret);

		for(int j = 0; j < ret_num_devices; j++){
			cout << "j: " << j << endl;
			cl_command_queue command_queue = clCreateCommandQueue(context, device_id[j], 0, &ret);
			OPENCL_CHECK(ret);

			cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, len * sizeof(int), NULL, &ret);
			OPENCL_CHECK(ret);
			cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, len * sizeof(int), NULL, &ret);
			OPENCL_CHECK(ret);
			cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, len * sizeof(int), NULL, &ret);
			OPENCL_CHECK(ret);

			ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0, len * sizeof(int), a, 0, NULL, NULL);
			ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, len * sizeof(int), b, 0, NULL, NULL);

			cl_program program = clCreateProgramWithSource(context, 1, (const char **)&str, NULL, &ret);
			OPENCL_CHECK(ret);

			ret = clBuildProgram(program, ret_num_devices, device_id, NULL, NULL, NULL);
			OPENCL_CHECK(ret);

			cl_kernel kernel = clCreateKernel(program, "vec_add", &ret);
			OPENCL_CHECK(ret);

			ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
			ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
			ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);

			size_t global_item_size[1];
			global_item_size[0] = len;
			size_t local_item_size = 64;
			ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_item_size, NULL, 0, NULL, NULL);
			OPENCL_CHECK(ret);

			int *c = (int *)malloc(sizeof(int) * len);
			ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, len * sizeof(int), (void *)c, 0, NULL, NULL);
			OPENCL_CHECK(ret);

			ret = clFlush(command_queue);
			ret = clFinish(command_queue);
			ret = clReleaseKernel(kernel);
			ret = clReleaseProgram(program);
			ret = clReleaseMemObject(a_mem_obj);
			ret = clReleaseMemObject(b_mem_obj);
			ret = clReleaseMemObject(c_mem_obj);
			ret = clReleaseCommandQueue(command_queue);

			cout << "a: ";
			for(int i = 0; i < len; i++){
				cout << a[i] << " ";
			}
			cout << endl;

			cout << "c: ";
			for(int z = 0; z < len; z++){
				cout << c[z] << " ";
			}
			cout << endl;
			free(c);
		} 
		ret = clReleaseContext(context);
		free(device_id); 
	}
	free(platform_id);
	free(a);
	free(b);

	return 0;
}