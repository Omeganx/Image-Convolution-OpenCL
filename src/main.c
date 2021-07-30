#include "../headers/CL_handler.h"
#include <CL/cl.h>
#include "../headers/performancelogger.h"
#include <math.h>
#define HALF_SIZE 8

int main(int argc, char **argv)
{
  startTimeLog("all");
  int width = 5184;
	int height = 3456;
	//this part is just to replace the raw image read that I removed.
	unsigned char *image_data = (unsigned char*)malloc(3*width*height*sizeof(unsigned char));	

	cl_handler handler;
	cl_handler_init(&handler, 0);
  cl_handler_info(&handler);

	cl_mem img_buffer = CL_CreateBufferFrom(handler.context, CL_MEM_READ_WRITE, sizeof(unsigned char) * 3 * width * height, image_data);
	cl_mem temp = CL_CreateBuffer(handler.context, CL_MEM_READ_WRITE, sizeof(float) * width * height);
	cl_mem tempImage = CL_CreateBuffer(handler.context, CL_MEM_READ_WRITE, sizeof(float) * width * height);
	cl_mem outputImage = CL_CreateBuffer(handler.context, CL_MEM_READ_WRITE, sizeof(float) * width * height);

	cl_mem inpImage = CL_Create2DImage(handler.context, CL_MEM_READ_WRITE, width, height, CL_INTENSITY, CL_FLOAT);
	cl_mem outImage = CL_Create2DImage(handler.context, CL_MEM_READ_WRITE, width, height, CL_INTENSITY, CL_FLOAT);

	float *grey = (float *)malloc(width * height * sizeof(float));
	size_t global[2] = {width, height};
	kernel_size ksize = getKernelSize(2, 0, global, NULL);

	int half_size = HALF_SIZE;
	float filter[(2 * half_size + 1) * (2 * half_size + 1)];
	float sigma = 20;
	float s = 0.0f;
	for (int x = 0; x <= 2 * half_size; x++)
	{
		for (int y = 0; y <= 2 * half_size; y++)
		{
			int dy = y - half_size;
			int dx = x - half_size;
			filter[y * (2 * half_size + 1) + x] = exp(-0.5 * (dx * dx + dy * dy) / sigma);
			s += filter[y * (2 * half_size + 1) + x];
		}
	}

	for (int x = 0; x <= 2 * half_size; x++)
	{
		for (int y = 0; y <= 2 * half_size; y++)
		{
			int dy = y - half_size;
			int dx = x - half_size;
			filter[y * (2 * half_size + 1) + x] = exp(-0.5 * (dx * dx + dy * dy) / sigma) / s;
		}
	}

	float filter_1D[2 * half_size + 1];
	s = 0.0f;

	for (int x = 0; x <= 2 * half_size; x++)
	{
		int dx = x - half_size;
		s += exp(-0.5 * (dx * dx) / sigma);
	}
	for (int x = 0; x <= 2 * half_size; x++)
	{
		int dx = x - half_size;
		filter_1D[x] = exp(-0.5 * (dx * dx) / sigma) / s;
	}

	cl_mem filtr = CL_CreateBufferFrom(handler.context, CL_MEM_READ_ONLY, sizeof(float) * (half_size * 2 + 1) * (half_size * 2 + 1), filter);
	cl_mem filtr_1d = CL_CreateBufferFrom(handler.context, CL_MEM_READ_ONLY, sizeof(float) * (2 * half_size + 1), filter_1D);

	startTimeLog("LaunchingKernels");
	queueKernel(&handler, "../kernel/image.cl", "preprocess", ksize, 4,
				&img_buffer, sizeof(cl_mem),
				&inpImage, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int));


	queueKernel(&handler, "../kernel/image.cl", "preprocess_buffer", ksize, 4,
				&img_buffer, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int));

	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_2D_image", ksize, 6,
				&inpImage, sizeof(cl_mem),
				&outImage, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr, sizeof(cl_mem),
				&half_size, sizeof(int));

	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_2D", ksize, 6,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr, sizeof(cl_mem),
				&half_size, sizeof(int));

	size_t local[2] = {16, 16};
	ksize = getKernelSize(2, 0, global, local);
	printf("local size is %li while the max local size is %i\n", (local[0] + 2 * half_size) * (local[1] + 2 * half_size) * sizeof(float), CL_DEVICE_LOCAL_MEM_SIZE);
	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_2D_cache", ksize, 7,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				NULL, (local[0] + 2 * half_size) * (local[1] + 2 * half_size) * sizeof(float),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr, sizeof(cl_mem),
				&half_size, sizeof(int));

	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_2D_cache_2", ksize, 7,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				NULL, (local[0] + 2 * half_size) * (local[1] + 2 * half_size) * sizeof(float),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr, sizeof(cl_mem),
				&half_size, sizeof(int));

	ksize = getKernelSize(2, 0, global, NULL);

	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_1D_pass1", ksize, 7,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&temp, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr_1d, sizeof(cl_mem),
				&half_size, sizeof(int));

	
	size_t local2[2] = {16, 1};
	ksize = getKernelSize(2, 0, global, local2);
	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_1D_pass1_cache", ksize, 8,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&temp, sizeof(cl_mem),
				NULL, (local2[0]+2*half_size)*sizeof(float),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr_1d, sizeof(cl_mem),
				&half_size, sizeof(int));

queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_1D_pass1_cache_nothing", ksize, 8,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&temp, sizeof(cl_mem),
				NULL, (local2[0]+2*half_size)*sizeof(float),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr_1d, sizeof(cl_mem),
				&half_size, sizeof(int));
	
	ksize = getKernelSize(2, 0, global, NULL);
	queueKernel(&handler, "../kernel/image.cl", "convolve_gauss_blur_1D_pass2", ksize, 7,
				&outputImage, sizeof(cl_mem),
				&tempImage, sizeof(cl_mem),
				&temp, sizeof(cl_mem),
				&width, sizeof(int),
				&height, sizeof(int),
				&filtr_1d, sizeof(cl_mem),
				&half_size, sizeof(int));

	addEventProfiling(&handler);
	stopTimeLog("LaunchingKernels");

	CL_ReadBufferTo(&handler, outputImage, sizeof(float) * width * height, grey);
	//CL_Read2DImage(handler.command_queue, outImage, width, height, grey);

	clReleaseMemObject(img_buffer);
	clReleaseMemObject(tempImage);
	clReleaseMemObject(outputImage);
	clReleaseMemObject(temp);

	clReleaseMemObject(inpImage);
	clReleaseMemObject(outImage);

	cl_handler_clear(&handler);

  stopTimeLog("all");
  displayLogs();
  freeLogs();

  return 0;
}
