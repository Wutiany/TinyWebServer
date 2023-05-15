#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}
sort_timer_lst::~sort_timer_lst()
{
    //wty 释放定时器内存
    util_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    //wty head为NULL则将当前timer设置成头尾
    if (!head)
    {
        head = tail = timer;
        return;
    }
    //wty sort timer pointer， 头插法
    if (timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    //wty 正序，直接返回
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    //wty 当前节点为定时器链表的头结点，要调整头结点，则需要对后续的节点进行调整
    if (timer == head)
    {
        //wty 头结点断链，将头结点指向next节点
        head = head->next;
        head->prev = NULL;
        //wty 将timer节点脱离，然后重新加入到链表中进行调整
        timer->next = NULL;
        //wty next节点作为头结点
        add_timer(timer, head);
    }
    else
    {
        //wty 分离head，重新加入链表进行调整
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        //wty timer的next节点作为头结点
        add_timer(timer, timer->next);
    }
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp = head;
    //wty 对定时器链表进行超时清洗，将超时的定时器全部删除，同时删除定时器对应的客户端请求，以及请求计数，关闭套接字
    while (tmp)
    {
        //wty 不超时的节点不作处理
        if (cur < tmp->expire)
        {
            break;
        }
        //wty 超时后删除定时器以及在epoll中的事件
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void Utils::init(int timeslot)
{
    //wty 初始化定时器事件
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
//wty EPOLLONESHOT表示只触发一次，该事件被处理完毕之前，都不会再触发
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    //wty 结构用来设置和处理信号函数
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        //wty 用于指定在信号处理程序返回时是否自动重新启动被中断的系统调用
        sa.sa_flags |= SA_RESTART;
    //wty 用于将一个信号集中的所有信号都设置为1
    sigfillset(&sa.sa_mask);
    //wty sigprocmask(SIG_BLOCK, &sa, NULL);   可以通过该函数阻塞置1的信号，阻塞信号是不让系统被阻塞的信号中断
    //wty 注册信号处理函数与信号进行关联
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

//wty 删除epoll中的事件，同时关闭套接字，减少http_conn::m_user_count的计数
class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
