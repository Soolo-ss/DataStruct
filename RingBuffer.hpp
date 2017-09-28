#include <atomic>
#include <array>
#include <mutex>
#include <chrono>

using std::array;
using namespace std::chrono_literals;


//lock-free
//based on array ring buffer
template <typename T>
class RingBuffer
{
public:
	RingBuffer()
		: head(0), tail(0), cursor(0)
	{
	}

	RingBuffer(bool is_open_sleep)
		: head(0), tail(0), cursor(0), is_open_sleep_model(is_open_sleep)
	{
	}

	size_t size()
	{
		return buffer.size();
	}

	inline bool push(T t)
	{
		int t_head = 0;
		int t_tail = 0;

		do
		{
			t_head = head.load();
			t_tail = tail.load();

			if (t_head - t_tail > MAX_BUFFER_LEN - 1)
				return false;
		} while (!std::atomic_compare_exchange_strong(&head, &t_head, t_head + 1));

		buffer[t_head % MAX_BUFFER_LEN] = t;

		int t_cursor = cursor.load();

		while (!std::atomic_compare_exchange_strong(&cursor, &t_cursor, t_cursor + 1))
		{
			if (is_open_sleep_model)
				std::this_thread::sleep_for(0ms);
			t_cursor = cursor.load();
		}

		return true;
	}

	inline bool pop(T& t)
	{
		int t_tail = 0;
		int t_cursor = 0;

		do
		{
			t_tail = tail.load();
			t_cursor = cursor.load();

			if (t_cursor == t_tail)
				return false;
		} while (!std::atomic_compare_exchange_strong(&tail, &t_tail, t_tail + 1));

		int pos = t_tail % MAX_BUFFER_LEN;
		t = buffer[pos];
		buffer[pos] = 0;

		return true;
	}

private:
	const static int MAX_BUFFER_LEN = 14600;
	
	bool is_open_sleep_model = false;

	std::atomic_int head;
	std::atomic_int cursor;
	std::atomic_int tail;
	array<T, MAX_BUFFER_LEN> buffer;
};