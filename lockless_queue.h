#ifndef LOCKLESS_QUEUE
#define LOCKLESS_QUEUE

#include <memory>//for the shared pointers
#include <condition_variable>

namespace threading
{
	template<typename T>
	class lockless_queue
	{
	private:
		template<typename U>
		class node
		{
			friend class lockless_queue;
			node(const U data) :
				data(new U(data)),
				next(nullptr),
				isDummy(false)
			{}
		private:
			node(bool isDummy) :
				data(nullptr),
				next(nullptr),
				isDummy(isDummy)
			{}
		public:
			const bool isDummy;
			std::shared_ptr<U> data;
			std::shared_ptr<node<U>> next;
		};

	public:
		lockless_queue()
			:
			m_head(new node<T>(true)),
			m_running(true)
		{}

		~lockless_queue()
		{
			m_running = false;
			m_newDataWaiter.notify_all();
		}

		//adds a new element to the end of the array
		void produce(const T &&data)
		{
			//the new node to be added at the end of the array
			std::shared_ptr<node<T>> l_newNode(new node<T>(std::forward<const T&&>(data)));
			//value to compare the next of the last node with
			std::shared_ptr<node<T>> l_expectedNullPointer;

			//pointer to the last node
			std::shared_ptr<node<T>> l_lastNode;
			std::shared_ptr<node<T>> l_nextLastNode = (std::atomic_load(&m_head));

			do
			{
				l_lastNode = l_nextLastNode;
				l_nextLastNode = std::atomic_load(&(l_lastNode->next));
			} while (l_nextLastNode);

			while (!std::atomic_compare_exchange_weak(&(l_lastNode->next), &l_expectedNullPointer, l_newNode))
			{
				l_lastNode = l_expectedNullPointer;
				l_expectedNullPointer.reset();
			}

			//notify if this is the first node inserted into an empty queue
			if (l_lastNode->isDummy)
				m_newDataWaiter.notify_one();
		}

		//Removes an element from the end of the array
		std::shared_ptr<T> consume(bool blockingCall = false)
		{

			//the pointer to the element we will consume
			std::shared_ptr<node<T>> l_head = std::atomic_load(&m_head);
			std::shared_ptr<node<T>> l_snack = std::atomic_load(&(l_head->next));

			do
			{
				//Check if the first node is null
				while (!l_snack)
				{   //and if it is :
					if (blockingCall && m_running)//And this is a blocking call,
					{
						std::unique_lock<std::mutex> l_newDataWaiterLock(m_newDataWaiterMutex);
						while (!l_head->next)
						{
							m_newDataWaiter.wait(l_newDataWaiterLock);//we block until

							if (!this || !m_running)//break if the object was destroyed during the wait
								return nullptr;

							l_snack = std::atomic_load(&(l_head->next));
						}// the load yields a head that is not null(to avoid unnecessary calls on spurious wake ups)
					}
					else//And this is not a blocking call we 
					{
						return nullptr;//return null	
					}
				}
			}
			/*Note that if the atomic CAS fails The new l_snack gets updated.
			*/
			while (!std::atomic_compare_exchange_weak(&(l_head->next), &l_snack, l_snack->next));


			if (l_snack)
				return l_snack->data;
			else
				return std::shared_ptr<T>();
		}

	private:
		//should be used as atomic
		std::shared_ptr<node<T>> m_head;
		std::mutex m_newDataWaiterMutex;
		std::condition_variable m_newDataWaiter;
		bool m_running;
	};

}
#endif // !LOCKLESS_QUEUE
