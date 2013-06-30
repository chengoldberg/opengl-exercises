// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string>

// OpenCL includes
#include <CL/cl.h>
#include <CL/cl_gl.h>

#include <iostream>
#include <fstream>
#include <sstream>

class CommonCL
{
public:

	static cl_context createContext()
	{
		cl_int errNum;
		cl_uint numPlatforms;
		cl_platform_id firstPlatformId;
		cl_context context;

		// Get first platform - don't care about rest
		errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
		if(errNum != CL_SUCCESS) 
		{
			std::cerr << "Failed to obtain platform list" << std::endl;
			return NULL;
		}

		// prepare context description
		cl_context_properties contextProperties[] = 
		{
			CL_CONTEXT_PLATFORM, (cl_context_properties) firstPlatformId,
			CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
			0 // zero-teriminated
		};
		
		// Create device list
		cl_uint devNum;
		errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, 0, NULL, &devNum);		
		cl_device_id* devices = new cl_device_id[devNum];
		errNum = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, devNum, devices, NULL);

		// Create the context
		context = clCreateContext(contextProperties, 1, &devices[0], NULL, NULL, &errNum);
		//context = clCreateContext(NULL, 1, &devices[0], NULL, NULL, &errNum);

		delete devices;

		if(errNum != CL_SUCCESS)
		{
			context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU, NULL, NULL, &errNum);
			if(errNum != CL_SUCCESS)
			{
				std::cerr << "Failed to create CL context" << std::endl;
				return NULL;
			}
		}
		return context;
	}

	static cl_device_id obtainFirstDeviceFromContext(cl_context context)
	{
		cl_int errNum;
		cl_device_id device;
		cl_device_id *devices;

		// Get the devices associated with the context
		size_t deviceBufferSize;
		errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
		if(errNum != CL_SUCCESS)
		{
			std::cerr << "Failed to obtain devices from given context" << std::endl;
			return NULL;
		}
		devices = new cl_device_id[deviceBufferSize/sizeof(cl_device_id)];	
		
		errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
		if(errNum != CL_SUCCESS)
		{
			std::cerr << "Failed to obtain devices from given context" << std::endl;
			delete devices;
			return NULL;
		}

		// get the first device
		device = devices[0];
		delete devices;

		return device;
	}

	static cl_command_queue createCommandQueue(cl_context context, cl_device_id device)
	{
		cl_command_queue commandQueue;
		
		if(device == NULL)
		{
			// Get any device associated with the context
			device = obtainFirstDeviceFromContext(context);		
		}
		if(device == NULL)
		{
			std::cerr << "Failed to obtain devices from given context" << std::endl;
			return NULL;
		}
		
		// Just associate command queue with first device we get
		commandQueue = clCreateCommandQueue(context, device, 0, NULL);
		if(commandQueue == NULL)
		{
			std::cerr << "Failed to create command queue for context" << std::endl;
			return NULL;
		}

		return commandQueue;
	}

	static cl_program createProgram(cl_context context, cl_device_id device, const char* filename)
	{
		cl_int errNum;
		cl_program program;

		std::string programSource;

		if(!readTextFromFile(filename, &programSource))
		{
			std::cerr << "Failed to read source from file" << std::endl;
			return NULL;
		}

		const char* programSourceChar = programSource.c_str();
		program = clCreateProgramWithSource(context, 1, &programSourceChar, NULL, NULL);
		if(program == NULL)
		{
			std::cerr << "Failed to create program from source" << std::endl;	
			return NULL;
		}

		errNum = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
		if(errNum != CL_SUCCESS)
		{
			std::cerr << "Failed to build program" << std::endl;	
			
			char buildLog[16000];
			clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);

			std::cerr << "Build log:" << buildLog << std::endl;	
			clReleaseProgram(program);
			return NULL;
		}
		return program;
	}

	static cl_kernel createKernel(cl_program program, const char* kernelName)
	{
		return clCreateKernel(program, kernelName, NULL);
	}

protected:
	static bool readTextFromFile(const char* filename, std::string* result)
	{
		std::ifstream inStream(filename, std::ios::in);
		if(!inStream.is_open())
			return false;
		std::ostringstream outStream;
		outStream << inStream.rdbuf();

		*result = outStream.str();
		inStream.close();
		
		return true;
	}
};

