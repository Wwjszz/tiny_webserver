#include "buffer.h"

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

// Test for append and retrieve
TEST(BufferTest, AppendRetrieveTest) {
  buffer buf;

  // Test appending and retrieving strings
  std::string data1 = "Hello";
  std::string data2 = "World";

  buf.append(data1);
  buf.append(data2);

  // Expect the total readable bytes to match the size of both strings
  EXPECT_EQ(buf.readable_bytes(), data1.size() + data2.size());

  // Retrieve first string and check its value
  std::string result1 = buf.retrieve_as_string(data1.size());
  EXPECT_EQ(result1, data1);

  // Retrieve second string and check its value
  std::string result2 = buf.retrieve_as_string(data2.size());
  EXPECT_EQ(result2, data2);

  // Expect buffer to be empty now
  EXPECT_EQ(buf.readable_bytes(), 0);
}

// Test for shrinking the buffer
TEST(BufferTest, ShrinkTest) {
  buffer buf;

  std::string data = "TestData";
  buf.append(data);

  // Shrink the buffer with some reserve space
  buf.shrink_(100);

  // Expect readable bytes to be the same and internal capacity to be reduced
  EXPECT_EQ(buf.readable_bytes(), data.size());
  EXPECT_GE(buf.internal_capacity_(), data.size() + 100);
}

// Test for reading from file descriptor (pipe)
TEST(BufferTest, ReadFdTest) {
  int pipefd[2];
  pipe(pipefd);

  // Write some data to the pipe
  std::string input = "Hello, World!";
  write(pipefd[1], input.c_str(), input.size());

  buffer buf;
  int Errno = 0;

  // Read from the pipe
  ssize_t bytes_read = buf.read_fd(pipefd[0], Errno);

  EXPECT_EQ(bytes_read, input.size());
  EXPECT_EQ(buf.readable_bytes(), input.size());

  // Retrieve the read data and compare with input
  std::string output = buf.retrieve_all_as_string();
  EXPECT_EQ(output, input);

  close(pipefd[0]);
  close(pipefd[1]);
}

// Test for writing to file descriptor (pipe)
TEST(BufferTest, WriteFdTest) {
  int pipefd[2];
  pipe(pipefd);

  buffer buf;
  std::string data = "BufferData";

  // Append data to the buffer
  buf.append(data);

  int Errno = 0;

  // Write buffer data to the pipe
  ssize_t bytes_written = buf.write_fd(pipefd[1], Errno);

  EXPECT_EQ(bytes_written, data.size());
  EXPECT_EQ(buf.readable_bytes(),
            0);  // Expect buffer to be empty after writing

  // Read from pipe and check the content
  char read_buffer[100] = {0};
  read(pipefd[0], read_buffer, sizeof(read_buffer));

  EXPECT_EQ(std::string(read_buffer, bytes_written), data);

  close(pipefd[0]);
  close(pipefd[1]);
}

// Test for ensure_writeable and make_space_
TEST(BufferTest, EnsureWriteableTest) {
  buffer buf;

  std::string data = "1234567890";
  buf.append(data);

  // Ensure there is enough space to write more data
  size_t new_data_len = 1024;
  buf.ensure_writeable(new_data_len);

  // Write more data
  std::string new_data = std::string(new_data_len, 'a');
  buf.append(new_data);

  EXPECT_EQ(buf.readable_bytes(), data.size() + new_data_len);
  EXPECT_GE(buf.writeable_bytes(), 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
