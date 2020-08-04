
//#include<device_launch_parameters.h>
//#include<cuda.h>
//#include<vector_types.h>
//#include<math.h>
//#include<cuda_runtime.h>


__global__ void simple_vbo_kernel(float4 *pos, unsigned int width, unsigned int height, float time)
{
	unsigned int x = blockIdx.x*blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y*blockDim.y + threadIdx.y;

	// calculate uv coordinates
	float u = x / (float)width;
	float v = y / (float)height;
	//u = u * 8.0f - 4.0f;
	//v = v * 8.0f - 4.0f;

	u = u * 3.0f - 1.5f;
	v = v * 3.0f - 1.5f;

	// calculate simple sine wave pattern
	//float freq = 4.0f;
	//float w = sinf(u*freq) * cosf(v*freq) * 0.5f;
	////float w = 0.0f;
	//// write output vertex 
	//pos[y*width + x] = make_float4(u, w, v, 1.0f);
	float freq = 4.0f;
	float w = sinf(u*freq + time)*cosf(v*freq + time)  * 0.5f;

	// write output vertex
	pos[y*width + x] = make_float4(u, w, v, 1.0f);

}


void launchCUDAKernel(float4 *pos, unsigned int mesh_width, unsigned int mesh_height, float time)
{
	dim3 block(8, 8, 1);
	dim3 grid(mesh_width / block.x, mesh_height / block.y, 1.0);
	simple_vbo_kernel << < grid, block >> > (pos, mesh_width, mesh_height, time);

}
