#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//wty 封装的信号量类
class sem
{
public:
    sem()
    {
        //wty pshared 信号量共享方式，信号量是一种计数器
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            //wty 初始化不成功抛出异常
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        //wty 銷毀信號量指針
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        //wty 等待信号量>0则减少信号量（1）并返回，如果=0，线程阻塞等待其他线程调用sem_post增加信号量的值
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        //wty 增加信号量的值，唤醒等待线程
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

//wty 封装的posix标准的互斥锁
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

//wty 封装的条件变量，作用是把释放互斥锁到休眠当做一个原子操作
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        //wty 正常为0返回值,会首先对互斥锁解锁，但是调用前需要先对线程进行加锁
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
