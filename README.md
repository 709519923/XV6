'xargs' is the most difficult part that I cost almost 4 days for searching so much reference, some are more complicated that implemented this function with some UNIX's features, others uses `pointer to pointer` for reading from standard input. I almost lost in it. Luckily, I find an easy code for this function with comment in details and without `pointer to pointer`.
Here, I put some refference for this function. They are good materias to know `string` and `pointer` .

https://www.howtogeek.com/435903/what-are-stdin-stdout-and-stderr-on-linux/
https://clownote.github.io/2021/02/24/xv6/Xv6-Lab-Utilities/#xargs
https://www.cnblogs.com/weijunji/p/XV6-study-01.html
https://blog.csdn.net/Valishment/article/details/125357588
https://blog.csdn.net/weixin_48283247/article/details/120602005
------------------------------28/12/2022-----------------------------------
This is a self monolog from a MSc. student in CUHK.
My name is DING Cong, and this is my first time to learn OS.
Even through the department have no plan setting OS-related course, I still want to learn it from scratch.
The first term is over & I'm able to do self-learning during the gap time.
when I come to HK, and have access to english textbook, open lecture. I found that it is absolutely a heaven for us to learn. However, time is limited(just 1year), fee is even extremely expensive(230k HKD).
I want to be more competitative when I graduate from CUHK and get a CS-related job. 
Chinamainland's computer environment is mostly softwareized. Most of students just care about doing software engineer etc. But is CS just left APP in China?
I think time for software-dominated is over. Engineer should be respected whatever they are working for. CS are still have many branches for research.
OS is one of them. I hope in the future, students majoring in engineering won't be worry about they salary and be happy from their intests in China.

Well, I just wanna make a complain.
mit6.828 is so difficult and I still at the beginning of sleep after watching the materials for one week.(╥_╥)
------------------------------21/12/2022-----------------------------------




xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Silas Boyd-Wickizer, Anton Burtsev, Ian Chen, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi, eyalz800, Nelson
Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, flespark,
Peter Froehlich, Yakir Goaron,Shivam Handa, Matt Harvey, Bryan Henry,
jaichenhengjie, Jim Huang, Matúš Jókay, Alexander Kapshuk, Anders
Kaseorg, kehao95, Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt,
Eddie Kohler, Vadim Kolontsov , Austin Liew, l0stman, Pavan
Maddamsetti, Imbar Marinescu, Yandong Mao, , Matan Shabtay, Hitoshi
Mitake, Carmi Merimovich, Mark Morrissey, mtasm, Joel Nider,
OptimisticSide, Greg Price, Jude Rich, Ayan Shafqat, Eldar Sehayek,
Yongming Shen, Fumiya Shigemitsu, Cam Tenny, tyfkda, Warren Toomey,
Stephen Tu, Rafael Ubal, Amane Uehara, Pablo Ventura, Xi Wang, Keiichi
Watanabe, Nicolas Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy
Zheng, ZhUyU1997, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2020 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.S081, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".
