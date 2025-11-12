// commnads to create spirv files from shader & some info about spriv compliations

// command for vertex
C:\VulkanSDK\Vulkan\Bin\glslangValidator.exe -V -H -o shader.vert.spv shader.vert 

// command for fragment
C:\VulkanSDK\Vulkan\Bin\glslangValidator.exe -V -H -o shader.frag.spv shader.frag 


-V -> V for Vulkan
-H -> for Human Redable Dump
-o -> outfile name

the spirv compilers check shader file extenstion to determine what type of shader it is dealing with
so if we don't want to use .vert/.frag/.tesc/.tese/.geom/.comp as extensions then we need to add one more flag to command i.e "-S"

For vertex shader where shader file is vertex.glsl
C:\VulkanSDK\Vulkan\Bin\glslangValidator.exe -V -H -S vert -o shader.vert.spv vertex.glsl

For vertex shader where shader file is fragment.glsl
C:\VulkanSDK\Vulkan\Bin\glslangValidator.exe -V -H -S frag -o shader.vert.spv fragment.glsl
