
rm -rf *.o VK *.txt

g++ -c -I$HOME/VulkanSDK/Vulkan/x86_64/include -o vk.o vk.cpp
g++ -o VK vk.o -lX11 -lm -L$HOME/VulkanSDK/Vulkan/x86_64/lib -L. -lvulkan -lSphere
./VK