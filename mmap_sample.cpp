#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string>
#include <thread>
#include <vector>

// 线程非安全，需要加锁
std::vector<char *> async_buffer;
bool exit_async_thread = false;
// 初始化缓存大小
const size_t buffer_size = 15;
char *buffer_ptr = nullptr;
char *write_ptr = nullptr;
FILE *log_file = nullptr;

void async_log_thread()
{
	while (true)
	{
		while (!async_buffer.empty())
		{
			char *buffer = async_buffer.back();
			async_buffer.pop_back();
			printf("thread get buffer = %s\n", buffer);
			size_t input_len = strlen(buffer);
			printf("thread get buffer length%ld\n", input_len);
			size_t remainSize = buffer_size - (write_ptr - buffer_ptr);
			if (input_len >= remainSize)
			{
				// 同步文件
				printf("input_len >= remainSize\n");

				fwrite(buffer_ptr, sizeof(char) * (write_ptr - buffer_ptr), 1, log_file);
				fflush(log_file);

				memset(buffer_ptr, '\0', buffer_size);
				write_ptr = buffer_ptr;
			}

			memcpy(write_ptr, buffer, input_len);
			write_ptr += input_len;

			printf("剩余空间 = %ld\n", (buffer_size - (write_ptr - buffer_ptr)));
		}
		if (exit_async_thread)
		{
			break;
		}
	}
}

int main(int argc, char **argv)
{
	int buffer_fd = open("android_temp.log", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ftruncate(buffer_fd, buffer_size);
	lseek(buffer_fd, 0, SEEK_SET);

	buffer_ptr = (char *)mmap(NULL, buffer_size, PROT_WRITE | PROT_READ, MAP_SHARED, buffer_fd, 0);
	memset(buffer_ptr, '\0', buffer_size);

	// 设置写入指针
	write_ptr = buffer_ptr + 0;

	// 新建处理线程
	std::thread async_thread(async_log_thread);

	log_file = fopen("android_mmap.log", "ab+");

	char input[50];
	while (1)
	{
		printf("输入：");
		scanf("%s", input);
		if (strcmp(input, "q") == 0)
		{
			break;
		}
		async_buffer.push_back(input);
	};

	munmap(buffer_ptr, buffer_size);
	fclose(log_file);
	exit_async_thread = true;
	async_thread.join();
	printf("程序退出\n");

	return 0;
}
