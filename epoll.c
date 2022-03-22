/*
 * @Author: LZH
 * @Date: 2022-03-15 20:35:18
 * @LastEditTime: 2022-03-16 15:19:28
 * @Description: 
 * @FilePath: /MyFiles/4_高并发服务器/homework/3_epoll.c
 */

#include "head.h"
#include <sys/epoll.h>
#define MAXEVENTSIZE   100

int socket_create(int port) {
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 设置地址重用  一般不会立即关闭而经历TIME_WAIT的过程
    int reuse = 1;  // 0默认，不重用
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return -1;
    }
    if (listen(fd, 20) < 0) {
        return -1;
    }
    return fd;
}

int main(int argc, char *argv[]) {
    int port = 8899;
    char opt;
    while ((opt = getopt(argc, argv, "p:")) == -1) {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            DBG(YELLOW"<USAGE> %s -p port\n"NONE, argv[0]);
            break;
        }
    }
    
    int server_listen;
    if ((server_listen = socket_create(port)) == -1) {
        perror("socket_create");
        exit(1);
    }
    DBG(YELLOW"<D> socket create successful\n"NONE);


    int epollfd;
    if ((epollfd = epoll_create(1)) < 0) {
        perror("epoll create");
        exit(1);
    }

    struct epoll_event my_events[MAXEVENTSIZE], ev;
    ev.data.fd = server_listen;
    ev.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_listen, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    }
    DBG(YELLOW"<D> server listen fd add to epoll successful\n"NONE);

    for (;;) {
        int nfds = epoll_wait(epollfd, my_events, MAXEVENTSIZE, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < nfds; i++) {
            int fd = my_events[i].data.fd;
            if (fd == server_listen && (my_events[i].events & EPOLLIN)) {
                int new_fd = accept(fd, NULL, NULL);
                if (new_fd < 0) {
                    perror("accept");
                    exit(1);
                }
                DBG(GREEN"<ACCEPT> \n"NONE);
                ev.data.fd = new_fd;
                ev.events = EPOLLIN;   // TODO 考虑边缘
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_fd, &ev) < 0) {
                    perror("epoll_ctl");
                    exit(1);
                }
            } else if ( (my_events[i].events & EPOLLIN)) {
                // dowork
                char buff[4096] = {0};
                int rsize = recv(fd, buff, sizeof(buff), 0);
                if (rsize == 0) {
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
                        perror("epoll_ctl");
                        exit(1);
                    }
                    close(fd);
                    DBG(RED"<CLOSE>\n"NONE);
                    continue;
                }
                DBG(GREEN"<RECV> rsize=[%d]: %s\n"NONE, rsize, buff);
                for (int i = 0; i < rsize; i++) {
                    if (buff[i] <= 'Z' && buff[i] >= 'A') {
                        buff[i] += 'a' - 'A';
                    } else if (buff[i] <= 'z' && buff[i] >= 'a') {
                        buff[i] += 'A' - 'a';
                    }
                }
                if (send(my_events[i].data.fd, buff, rsize, 0) < 0) {
                    perror("send");
                    exit(1);
                }
            }
        }
    }


    return 0;
}
