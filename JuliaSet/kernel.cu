#include<device_launch_parameters.h>



#define DIMX 1920
#define DIMY 1080
dim3 grid(DIMX, DIMY);

struct CuComplex {
	float r;
	float i;

	__device__ CuComplex(float a, float b) :r(a), i(b) {}
	__device__ float magnitude2(void) {
		return r * r + i * i;
	}

	__device__ CuComplex operator*(const CuComplex& a)
	{
		return CuComplex(r*a.r - i * a.i, i*a.r + r * a.i);
	}

	__device__ CuComplex operator+(const CuComplex& a)
	{
		return CuComplex(r + a.r, i + a.i);
	}
};


__device__ int julia(int x, int y)
{
	const float scale = 1.5;
	float jx = scale * (float)(DIMX / 2 - x) / (DIMX / 2);
	float jy = scale * (float)(DIMY / 2 - y) / (DIMY / 2);

	CuComplex c(-0.8, 0.154);
	CuComplex a(jx, jy);

	int i = 0;
	for (i = 0; i < 200; i++)
	{
		a = a * a + c;
		if (a.magnitude2() > 1000)
			return 0;
	}
	return 1;
}

__global__ void kernel(unsigned char *ptr)
{
	int x = blockIdx.x;
	int y = blockIdx.y;
	int offset = x + y * gridDim.x;

	int juliaValue = julia(x, y);
	ptr[offset * 4 + 0] = 0;
	ptr[offset * 4 + 1] = 0;
	ptr[offset * 4 + 2] = 255 * juliaValue;
	ptr[offset * 4 + 3] = 255;

}

void launch_kernel(unsigned char  *pos)
{
	//execute the kernel
 // execute the kernel
	//dim3 block(8, 8, 1);	//8 * 8 *1 threads
	//dim3 grid(mesh_width / block.x, mesh_height / block.y, 1);	//1024 / 8 = 128	blocks , 1024 /8 = 128 blocks means(128*128 = 16384)blocks
	kernel << <grid, 1 >> > (pos);

	//simple_vbo_kernel << < grid, block >> > (pos, mesh_width, mesh_height, time);
}