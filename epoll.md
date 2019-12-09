## epoll源码的分析
### 1、linux提供了3个关键函数来操作epoll，分别是</br>

- ①epoll_create(),创建一个eventpoll对象，我的理解就是创建一个监听的句柄(联想到select函数的一个参数是fd的最大数目+1)
- ②epoll_ctl(),操作eventpoll对象
- ③epoll_wait(),从eventpoll对象中返回活跃的事件
而操作系统内部还有一个epoll_event_callback()的回调函数来调度epoll中的事件



### 2、 核心数据结构
- ①epitem 如图<br/>
 &emsp;&emsp;&emsp;&emsp;![结构图](https://pic1.xuehuaimg.com/proxy/csdn/https://img-blog.csdnimg.cn/20190716141454917.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L1JlcGx1c18=,size_16,color_FFFFFF,t_70)<br/>
eptiem主要包括两个成员变量,rbn和rdlink,前者是红黑树的节点，否则是双向链表的节点，也就是一个epitem对象既可以是红黑树的一个节点又可以是双向链表的一个节点，每个epitem中有一个event，对于event事件的查询就是对epitem的查询

```c++

struct epitem {
	RB_ENTRY(epitem) rbn;
	/*  RB_ENTRY(epitem) rbn等价于
	struct {											
		struct type *rbe_left;		//指向左子树
		struct type *rbe_right;		//指向右子树
		struct type *rbe_parent;	//指向父节点
		int rbe_color;			    //该节点的颜色
	} rbn
	*/

	LIST_ENTRY(epitem) rdlink;
	/* LIST_ENTRY(epitem) rdlink等价于
	struct {									
		struct type *le_next;	//指向下个元素
		struct type **le_prev;	//前一个元素的地址
	}*/

	int rdy; //判断该节点是否同时存在与红黑树和双向链表中
	
	int sockfd; //socket句柄
	struct epoll_event event;  //存放用户填充的事件
};
```

- ②eventpoll
 &emsp;&emsp;&emsp;&emsp;![结构图](https://pic1.xuehuaimg.com/proxy/csdn/https://img-blog.csdnimg.cn/20190716165055592.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L1JlcGx1c18=,size_16,color_FFFFFF,t_70)<br/>

eventpoll主要包括两个成员，rbr和fdlist，前者指向红黑树，否则指向双向链表。也就是说一个eventpoll对应着一个epitem结构的两个容器(红黑树和双向链表)
```c++
struct eventpoll {
	/*
	struct ep_rb_tree {
		struct epitem *rbh_root; 			
	}
	*/
	ep_rb_tree rbr;      //rbr指向红黑树的根节点
	
	int rbcnt; //红黑树中节点的数量（也就是添加了多少个TCP连接事件）
	
	LIST_HEAD( ,epitem) rdlist;    //rdlist指向双向链表的头节点；
	/*	这个LIST_HEAD等价于 
		struct {
			struct epitem *lh_first;
		}rdlist;
	*/
	
	int rdnum; //双向链表中节点的数量（也就是有多少个TCP连接来事件了）

	// ...略...
	
};
```
### 3、 关键函数
- (1)epoll_create()
```c++

//创建epoll对象，包含一颗空红黑树和一个空双向链表
int epoll_create(int size) {
	//与很多内核版本一样，size参数没有作用，只要保证大于0即可
	if (size <= 0) return -1;
	
	nty_tcp_manager *tcp = nty_get_tcp_manager(); //获取tcp对象
	if (!tcp) return -1;
	
	struct _nty_socket *epsocket = nty_socket_allocate(NTY_TCP_SOCK_EPOLL);
	if (epsocket == NULL) {
		nty_trace_epoll("malloc failed\n");
		return -1;
	}

	// 1° 开辟了一块内存用于填充eventpoll对象
	struct eventpoll *ep = (struct eventpoll*)calloc(1, sizeof(struct eventpoll));
	if (!ep) {
		nty_free_socket(epsocket->id, 0);
		return -1;
	}

	ep->rbcnt = 0;

	// 2° 让红黑树根指向空
	RB_INIT(&ep->rbr);       //等价于ep->rbr.rbh_root = NULL;

	// 3° 让双向链表的头指向空
	LIST_INIT(&ep->rdlist);  //等价于ep->rdlist.lh_first = NULL;

	// 4° 并发环境下进行互斥
	// ...该部分代码与主线逻辑无关，可自行查看...

	//5° 保存epoll对象
	tcp->ep = (void*)ep;
	epsocket->ep = (void*)ep;

	return epsocket->id;
}
```
包括的步骤就是<br/>
##### 1、创建eventpoll对象
##### 2、eventpoll对象的rbr指向空
##### 3、eventpoll的rdlist指向空
##### 4、并发环境下互斥
##### 5、保存eventpoll对象
##### 6、返回eventpoll对象的句柄(id)


- (2)epoll_ctl()<br/>
根据对于fd的操作opEPOLL_CTL_ADD,EPOLL_CTL_MOD和EPOLL_CTL_DEL决定将epitem对象插入红黑树还是更新红黑树还是删除红黑树
```c++
//往红黑树中加每个tcp连接以及相关的事件
int epoll_ctl(int epid, int op, int sockid, struct epoll_event *event) {

	nty_tcp_manager *tcp = nty_get_tcp_manager();
	if (!tcp) return -1;

	nty_trace_epoll(" epoll_ctl --> 1111111:%d, sockid:%d\n", epid, sockid);
	struct _nty_socket *epsocket = tcp->fdtable->sockfds[epid];

	if (epsocket->socktype == NTY_TCP_SOCK_UNUSED) {
		errno = -EBADF;
		return -1;
	}

	if (epsocket->socktype != NTY_TCP_SOCK_EPOLL) {
		errno = -EINVAL;
		return -1;
	}

	nty_trace_epoll(" epoll_ctl --> eventpoll\n");

	struct eventpoll *ep = (struct eventpoll*)epsocket->ep;
	if (!ep || (!event && op != EPOLL_CTL_DEL)) {
		errno = -EINVAL;
		return -1;
	}

	if (op == EPOLL_CTL_ADD) {
		//添加sockfd上关联的事件
		pthread_mutex_lock(&ep->mtx);

		struct epitem tmp;
		tmp.sockfd = sockid;
		struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp); //先在红黑树上找，根据key来找，也就是这个sockid，找的速度会非常快
		if (epi) {
			//原来有这个节点，不能再次插入
			nty_trace_epoll("rbtree is exist\n");
			pthread_mutex_unlock(&ep->mtx);
			return -1;
		}

		//只有红黑树上没有该节点【没有用过EPOLL_CTL_ADD的tcp连接才能走到这里】；

		//(1)生成了一个epitem对象，这个结构对象，其实就是红黑的一个节点；
		epi = (struct epitem*)calloc(1, sizeof(struct epitem));
		if (!epi) {
			pthread_mutex_unlock(&ep->mtx);
			errno = -ENOMEM;
			return -1;
		}
		
		//(2)把socket(TCP连接)保存到节点中；
		epi->sockfd = sockid;  //作为红黑树节点的key，保存在红黑树中

		//(3)我们要增加的事件也保存到节点中；
		memcpy(&epi->event, event, sizeof(struct epoll_event));

		//(4)把这个节点插入到红黑树中去
		epi = RB_INSERT(_epoll_rb_socket, &ep->rbr, epi); //实际上这个时候epi的rbn成员就会发挥作用，如果这个红黑树中有多个节点，那么RB_INSERT就会epi->rbi相应的值：可以参考图来理解
		assert(epi == NULL);
		ep->rbcnt ++;
		
		pthread_mutex_unlock(&ep->mtx);

	} else if (op == EPOLL_CTL_DEL) {
		pthread_mutex_lock(&ep->mtx);

		struct epitem tmp;
		tmp.sockfd = sockid;
		
		struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);//先在红黑树上找，根据key来找，也就是这个sockid，找的速度会非常快
		if (!epi) {
			nty_trace_epoll("rbtree no exist\n");
			pthread_mutex_unlock(&ep->mtx);
			return -1;
		}
		
		//只有在红黑树上找到该节点【用过EPOLL_CTL_ADD的tcp连接才能走到这里】；

		//从红黑树上把这个节点移除
		epi = RB_REMOVE(_epoll_rb_socket, &ep->rbr, epi);
		if (!epi) {
			nty_trace_epoll("rbtree is no exist\n");
			pthread_mutex_unlock(&ep->mtx);
			return -1;
		}

		ep->rbcnt --;
		free(epi);
		
		pthread_mutex_unlock(&ep->mtx);

	} else if (op == EPOLL_CTL_MOD) {
		struct epitem tmp;
		tmp.sockfd = sockid;
		struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp); //先在红黑树上找，根据key来找，也就是这个sockid，找的速度会非常快
		if (epi) {
			//红黑树上有该节点，则修改对应的事件
			epi->event.events = event->events;
			epi->event.events |= EPOLLERR | EPOLLHUP;
		} else {
			errno = -ENOENT;
			return -1;
		}

	} else {
		nty_trace_epoll("op is no exist\n");
		assert(0);
	}

	return 0;
}
```

- (3)epoll_wait()函数
逻辑是查看eventpoll对象中的双有事件，双向链表时候存在，因为根据前面说的可以知道，双向链表代表的是tcp连接中事件到来的个数，而红黑树表示的是tcp中连接的个数。
所以如果有节点，就把节点中的事件填充到用户传入的指向内存的指针中。如果没有节点，就用while循环等待一定时间，这里和timeout有关。直到有时间触发，操作系统会将事件插入双向链表中，使得(rdnum>0)(这个操作由epoll_event_callback函数完成)，程序跳出while循环，去双向链表中取数据

```c++
//到双向链表中去取相关的事件通知
int epoll_wait(int epid, struct epoll_event *events, int maxevents, int timeout) {

	nty_tcp_manager *tcp = nty_get_tcp_manager();
	if (!tcp) return -1;

	struct _nty_socket *epsocket = tcp->fdtable->sockfds[epid];

	struct eventpoll *ep = (struct eventpoll*)epsocket->ep;
	
    // ...此处主要是一些负责验证性工作的代码...

	//(1)当eventpoll对象的双向链表为空时，程序会在这个while中等待一定时间，
	//直到有事件被触发，操作系统将epitem插入到双向链表上使得rdnum>0时，程序才会跳出while循环
	while (ep->rdnum == 0 && timeout != 0) {
		// ...此处主要是一些与等待时间相关的代码...
		调用操作系统的回调函数
	}


	pthread_spin_lock(&ep->lock);

	int cnt = 0;

	//(1)取得事件的数量
	//ep->rdnum：代表双向链表里边的节点数量（也就是有多少个TCP连接来事件了）
	//maxevents：此次调用最多可以收集到maxevents个已经就绪【已经准备好】的读写事件
	int num = (ep->rdnum > maxevents ? maxevents : ep->rdnum); //哪个数量少，就取得少的数字作为要取的事件数量
	int i = 0;
	
	while (num != 0 && !LIST_EMPTY(&ep->rdlist)) { //EPOLLET

		//(2)每次都从双向链表头取得 一个一个的节点
		struct epitem *epi = LIST_FIRST(&ep->rdlist);

		//(3)把这个节点从双向链表中删除【但这并不影响这个节点依旧在红黑树中】
		LIST_REMOVE(epi, rdlink); 

		//(4)这是个标记，标记这个节点【这个节点本身是已经在红黑树中】已经不在双向链表中；
		epi->rdy = 0;  //当这个节点被操作系统 加入到 双向链表中时，这个标记会设置为1。

		//(5)把事件标记信息拷贝出来；拷贝到提供的events参数中
		memcpy(&events[i++], &epi->event, sizeof(struct epoll_event));
		
		num --;
		cnt ++;       //拷贝 出来的 双向链表 中节点数目累加
		ep->rdnum --; //双向链表里边的节点数量减1
	}
	
	pthread_spin_unlock(&ep->lock);

	//(5)返回 实际 发生事件的 tcp连接的数目；
	return cnt; 
}
```

- (4)epoll_event_callback()
当服务器中有以下5中情况时会调用epoll_event_callback函数
(1)客户端connect()接入,服务器处于SYN_RCVD状态
(2)三次握手完成,服务器处于ESTABLISHED状态
(3)客户端close()断开连接，服务器处于FIN_WAIT_1和FIN_WAIT_2状态
(4)客户端send/wait数据时，服务器可读写
(5)服务端发数据时
```c++

//当发生客户端三路握手连入、可读、可写、客户端断开等情况时，操作系统会调用这个函数，用以往双向链表中增加一个节点【该节点同时 也在红黑树中】
int epoll_event_callback(struct eventpoll *ep, int sockid, uint32_t event) {
	struct epitem tmp;
	tmp.sockfd = sockid;

	//(1)根据给定的key【这个TCP连接的socket】从红黑树中找到这个节点
	struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);
	if (!epi) {
		nty_trace_epoll("rbtree not exist\n");
		assert(0);
	}

	//(2)从红黑树中找到这个节点后，判断这个节点是否已经被连入到双向链表里【判断的是rdy标志】
	if (epi->rdy) {
		//这个节点已经在双向链表里，那无非是把新发生的事件标志增加到现有的事件标志中
		epi->event.events |= event;
		return 1;
	} 

	//走到这里，表示 双向链表中并没有这个节点，那要做的就是把这个节点连入到双向链表中

	nty_trace_epoll("epoll_event_callback --> %d\n", epi->sockfd);
	
	pthread_spin_lock(&ep->lock);

	//(3)标记这个节点已经被放入双向链表中，我们刚才研究epoll_wait()的时候，从双向链表中把这个节点取走的时候，这个标志被设置回了0
	epi->rdy = 1;  

	//(4)把这个节点链入到双向链表的表头位置
	LIST_INSERT_HEAD(&ep->rdlist, epi, rdlink);

	//(5)双向链表中的节点数量加1，刚才研究epoll_wait()的时候，从双向链表中把这个节点取走的时候，这个数量减了1
	ep->rdnum ++;

	pthread_spin_unlock(&ep->lock);
	pthread_mutex_lock(&ep->cdmtx);
	pthread_cond_signal(&ep->cond);
	pthread_mutex_unlock(&ep->cdmtx);

	return 0;
}
```
也就是将eventpoll指向的红黑树的节点插入双向链表中

### 4、 结论
epoll底层两个关键的数据结构,一个是eventpoll一个是epitem,其中evetnpoll两个变量rbr和rdlist。而epitem则是红黑树节点和双向链表节点的综合体，也就是说epitem的部分结构属于eventpoll里面，就是rbr和rdlist,但是epitem里面注册着用户的事件

- 用户调用epoll_create()时，创建一个eventpoll对象(包括红黑树和双向链表)
- 用户调用epoll_ctl(ADD)时，会在红黑树上增加节点(epitem对象)
- 操作系统非通过epoll_event_callback()来管理eventpoll的对象，一旦有事件出发，加入将eptiem的rdlist加入双向链表中
- 用户需要管理时，通过epoll_wait对eventpoll对象中的双向链表的epoll_event指针进行管理

