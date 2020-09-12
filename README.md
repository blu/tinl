Description
-----------

*This is Not Lisp* (**TINL**, proncounced *tai-nul*) is a minimalistic LISP-like language, using familiar s-expressions with essential restrictions:

* no lists
* no array, vector, string or function types -- just scalars
* no *defstruct* structured types either
* no *set/setq/setf* forms -- variable initializations are effectively Single Static Assignments (SSA)
* no quoted expressions
* no lambdas
* no NIL results -- all expressions must return a value
* no bignum numeric type -- only fixed-bitness integers
* no rational numeric type -- only floating-point fractions
* no binary or octal literals -- only decimal and hexidecimal (via *0x* prefix)
* no T/NIL predicate forms -- *ifzero/ifneg expr then-expr else-expr* instead
* no *dotimes* et al loop forms -- loops only via recursion

Built-in functions
------------------

`+ arg1 arg2 {..}` -- compute arg1 + arg2 + .. + argN; promote `int` args to `float` if at least one arg is `float`  
`- arg1 arg2 {..}` -- compute arg1 - arg2 - .. - argN; promote `int` args to `float` if at least one arg is `float`  
`* arg1 arg2 {..}` -- compute arg1 * arg2 * .. * argN; promote `int` args to `float` if at least one arg is `float`  
`/ arg1 arg2 {..}` -- compute arg1 / arg2 / .. / argN; promote `int` args to `float` if at least one arg is `float`  
`ifzero arg1 arg2 arg3` -- if arg1 is zero, compute arg2, otherwise compute arg3  
`ifneg arg1 arg2 arg3` -- if arg1 is negative, compute arg2, otherwise compute arg3  
`print arg` -- print arg and return arg; exert side-effect  
`readi32`, `readf32` -- read a scalar of the respective type from stdin

Interpreter considerations
--------------------------

TINL interpreter is implemented as an one-register Harvard stack machine:

* evaluations are returned via a single (implicit) register  
* variables and arguments are stored on a data stack  
* return addresses are stored on an (implicit) return stack

Example of correspondence between LISP and TINL
-----------------------------------------------

Print `n + 2` consecutive members of the Fibonacci sequence. Maximum code likeness sought.

Common LISP:

```lisp
	(defun fib(x y n)
		(print x) (if (zerop n) (print y) (fib y (+ x y) (- n 1))))
```

TINL:

```lisp
	(defun fib(x y n)
		(print x) (ifzero n (print y) (fib y (+ x y) (- n 1))))
```

Benchmarks
----------

| CPU                                               | factorial, time | fibonacci, time | remarks              |
| ------------------------------------------------- | --------------- | --------------- | -------------------- |
| MediaTek MT8163A (Cortex-A53 @ 1.5 GHz)           |  0.795s         |  6.312s         | clang++-3.9, aarch64 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  0.627s         |  5.299s         | g++-8.2, aarch64     |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.259s         |  2.123s         | g++-8.2 8.3, aarch64 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  0.766s         |  6.090s         | clang++-7.0, aarch32 |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.286s         |  2.349s         | clang++-7.0, aarch32 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  0.622s         |  5.370s         | g++-8.2, aarch32     |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.240s         |  2.193s         | g++-8.2, aarch32     |
| Intel Xeon E5-2687W (Sandy Bridge @ 3.1 GHz)      |  0.155s         |  1.350s         | clang++-3.9, amd64   |
| Intel Xeon E3-1270v2 (Ivy Bridge @ 1.6 GHz)       |  0.288s         |  2.616s         | clang++-3.7, amd64   |
| Marvell ARMADA 8040 (Cortex-A72 @ 1.3 GHz)        |  0.395s         |  3.460s         | g++-8.1, aarch64     |
| Baikal-T1 (P5600 @ 1.2 GHz)                       |  0.699s         |  5.670s         | g++-8.3, mips32r5    |
| Exynos 5422 (Cortex-A15 @ 2.0 GHz)                |  0.307s         |  3.371s         | g++-8.3, armv7l      |
| Tegra X1 (Cortex-A57 @ 1.428 GHz)                 |  0.450s         |  3.866s         | g++-8.2, aarch64     |
| Snapdragon 835 (Cortex-A73 @ 2.6GHz)              |  0.250s         |  1.997s         | g++-8.4, aarch64     |

| CPU                                               | factorial, Gclk | fibonacci, Gclk | remarks              |
| ------------------------------------------------- | --------------- | --------------- | -------------------- |
| MediaTek MT8163A (Cortex-A53 @ 1.5 GHz)           |  1.1925         |  9.4680         | clang++-3.9, aarch64 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  1.0659         |  9.0083         | g++-8.2, aarch64     |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.5439         |  4.4583         | g++-8.2 8.3, aarch64 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  1.3022         | 10.3530         | clang++-7.0, aarch32 |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.6006         |  4.9329         | clang++-7.0, aarch32 |
| MediaTek MT8173C (Cortex-A53 @ 1.7 GHz)           |  1.0574         |  9.1290         | g++-8.2, aarch32     |
| MediaTek MT8173C (Cortex-A72 @ 2.1 GHz)           |  0.5040         |  4.6053         | g++-8.2, aarch32     |
| Intel Xeon E5-2687W (Sandy Bridge @ 3.1 GHz)      |  0.4805         |  4.1850         | clang++-3.9, amd64   |
| Intel Xeon E3-1270v2 (Ivy Bridge @ 1.6 GHz)       |  0.4608         |  4.1856         | clang++-3.7, amd64   |
| Marvell ARMADA 8040 (Cortex-A72 @ 1.3 GHz)        |  0.5135         |  4.4980         | g++-8.1, aarch64     |
| Baikal-T1 (P5600 @ 1.2 GHz)                       |  0.8388         |  6.8040         | g++-8.3, mips32r5    |
| Exynos 5422 (Cortex-A15 @ 2.0 GHz)                |  0.6140         |  6.7420         | g++-8.3, armv7l      |
| Tegra X1 (Cortex-A57 @ 1.428 GHz)                 |  0.6426         |  5.5206         | g++-8.2, aarch64     |
| Snapdragon 835 (Cortex-A73 @ 2.6GHz)              |  0.6500         |  5.1922         | g++-8.4, aarch64     |

Logs
----

MediaTek MT8163A (ARM Cortex-A53 @ 1.5GHz, aarch64)
```
$ clang++-3.9 main.cpp -o test -std=c++11 -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -DNDEBUG -march=armv8-a -Wno-c++11-narrowing -Wno-switch -Wno-format-security && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0.795s
user    0m0.790s
sys     0m0.000s
$ echo "scale=4; 0.795 * 1.5" | bc
1.1925
$ time ./test bench_fib.tinl > /dev/null

real    0m6.312s
user    0m6.270s
sys     0m0.000s
$ echo "scale=4; 6.312 * 1.5" | bc
9.4680
```

MediaTek MT8173C (ARM Cortex-A53 @ 1.7GHz, aarch32)
```
$ clang++-7.0 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Os -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0x3 ./test bench_fac.tinl > /dev/null

real    0m0.766s
user    0m0.752s
sys     0m0.012s
$ echo "scale=4; 0.766 * 1.7" | bc
1.3022

$ time taskset 0x3 ./test bench_fib.tinl > /dev/null

real    0m6.090s
user    0m6.076s
sys     0m0.004s
$ echo "scale=4; 6.090 * 1.7" | bc
10.3530

$ g++-8.2 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0x3 ./test bench_fac.tinl > /dev/null

real    0m0.622s
user    0m0.616s
sys     0m0.004s
$ echo "scale=4; 0.622 * 1.7" | bc
1.0574

$ time taskset 0x3 ./test bench_fib.tinl > /dev/null

real    0m5.370s
user    0m5.364s
sys     0m0.004s
$ echo "scale=4; 5.370 * 1.7" | bc
9.1290
```

MediaTek MT8173C (ARM Cortex-A72 @ 2.1GHz, aarch32)
```
$ clang++-7.0 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Os -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0xc ./test bench_fac.tinl > /dev/null

real    0m0.286s
user    0m0.284s
sys     0m0.000s
$ echo "scale=4; 0.286 * 2.1" | bc
.6006

$ time taskset 0xc ./test bench_fib.tinl > /dev/null

real    0m2.349s
user    0m2.336s
sys     0m0.008s
$ echo "scale=4; 2.349 * 2.1" | bc
4.9329

$ g++-8.2 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0xc ./test bench_fac.tinl > /dev/null

real    0m0.240s
user    0m0.232s
sys     0m0.004s
$ echo "scale=4; 0.240 * 2.1" | bc
.5040

$ time taskset 0xc ./test bench_fib.tinl > /dev/null

real    0m2.193s
user    0m2.180s
sys     0m0.004s
$ echo "scale=4; 2.193 * 2.1" | bc
4.6053
```

MediaTek MT8173C (ARM Cortex-A53 @ 1.7GHz, aarch64)
```
$ g++-8.2 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0x3 ./test bench_fac.tinl > /dev/null

real    0m0.627s
user    0m0.620s
sys     0m0.000s
$ echo "scale=4; 0.627 * 1.7" | bc
1.0659
$ time taskset 0x3 ./test bench_fib.tinl > /dev/null

real    0m5.299s
user    0m5.280s
sys     0m0.010s
$ echo "scale=4; 5.299 * 1.7" | bc
9.0083
```

MediaTek MT8173C (ARM Cortex-A72 @ 2.1GHz, aarch64)
```
$ g++-8.2 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0xc ./test bench_fac.tinl > /dev/null

real    0m0.259s
user    0m0.250s
sys     0m0.000s
$ echo "scale=4; 0.259 * 2.1" | bc
.5439
$ g++-8.3 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time taskset 0xc ./test bench_fib.tinl > /dev/null

real    0m2.123s
user    0m2.108s
sys     0m0.004s
$ echo "scale=4; 2.123 * 2.1" | bc
4.4583
```

Intel Xeon E5-2687W (Sandy Bridge @ 3.1GHz, amd64)
```
$ clang++-3.9 main.cpp -o test -std=c++11 -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=native -Wno-format-security -Wno-switch -DNDEBUG && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0.155s
user    0m0.152s
sys     0m0.002s
$ echo "scale=4; 0.155 * 3.1" | bc
.4805
$ time ./test bench_fib.tinl > /dev/null

real    0m1.350s
user    0m1.347s
sys     0m0.001s
$ echo "scale=4; 1.350 * 3.1" | bc
4.1850
```

Intel Xeon E3-1270v2 (Ivy Bridge @ 1.6GHz, amd64)
```
$ clang++-3.7 main.cpp -o test -std=c++11 -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=native -Wno-format-security -Wno-switch -DNDEBUG && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0.288s
user    0m0.284s
sys     0m0.004s
$ echo "scale=4; 0.288 * 1.6" | bc
.4608

$ time ./test bench_fib.tinl > /dev/null

real    0m2.616s
user    0m2.610s
sys     0m0.004s
$ echo "scale=4; 2.616 * 1.6" | bc
4.1856
```

Marvell ARMADA 8040 (ARM Cortex-A72 @ 1.3GHz, aarch64)
```
$ g++-8.1 main.cpp -o test -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -std=c++11 -Wno-switch -Wno-format-security -Wno-div-by-zero -DNDEBUG && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0.395s
user    0m0.384s
sys     0m0.008s
$ echo "scale=4; 0.395 * 1.3" | bc
.5135
$ time ./test bench_fib.tinl > /dev/null

real    0m3.460s
user    0m3.448s
sys     0m0.008s
$ echo "scale=4; 3.460 * 1.3" | bc
4.4980
```

Baikal-T1 (P5600 @ 1.2GHz, mips32r5)
```
$ g++-8.3 main.cpp -o test -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -std=c++11 -Wno-switch -Wno-format-security -Wno-div-by-zero -march=p5600 -mtune=p5600 -DNDEBUG && strip ./test
$ time taskset 0x1 ./test bench_fac.tinl > /dev/null

real    0m0.699s
user    0m0.690s
sys     0m0.000s
$ echo "scale=4; 0.699 * 1.2" | bc
.8388
$ time taskset 0x1 ./test bench_fib.tinl > /dev/null

real    0m5.670s
user    0m5.660s
sys     0m0.000s
$ echo "scale=4; 5.670 * 1.2" | bc
6.8040
```

Exynos 5422 (Cortex-A15 @ 2.0GHz, armv7l)
```
$ g++-8.3 main.cpp -o test -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -DNDEBUG -mcpu=cortex-a15 -Wno-switch -Wno-format-security -Wno-div-by-zero && strip ./test
$ time taskset 0xf0 ./test bench_fac.tinl > /dev/null

real    0m0.307s
user    0m0.290s
sys     0m0.015s
$ echo "scale=4; 0.307 * 2.0" | bc
.6140
$ time taskset 0xf0 ./test bench_fib.tinl > /dev/null

real    0m3.371s
user    0m3.360s
sys     0m0.005s
$ echo "scale=4; 3.371 * 2.0" | bc
6.7420
```

Tegra X1 (Cortex-A57 @ 1.428GHz, aarch64)
```
$ g++-8.2 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -mcpu=cortex-a57 -DNDEBUG && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0,450s
user    0m0,448s
sys     0m0,000s
$ echo "scale=4; 0.450 * 1.428" | bc
.6426
$ time ./test bench_fib.tinl > /dev/null

real    0m3,866s
user    0m3,856s
sys     0m0,000s
$ echo "scale=4; 3.866 * 1.428" | bc
5.5206
```

Snapdragon 835
```
$ g++-8.4 main.cpp -o test -Wno-div-by-zero -Wno-switch -Wno-format-security -Ofast -fno-exceptions -fno-rtti -fstrict-aliasing -march=armv8-a -DNDEBUG && strip ./test
$ time ./test bench_fac.tinl > /dev/null

real    0m0.250s
user    0m0.234s
sys     0m0.016s
$ echo "scale=4; 0.250 * 2.6" | bc
.6500
$ time ./test bench_fib.tinl > /dev/null

real    0m1.997s
user    0m1.938s
sys     0m0.063s
$ echo "scale=4; 1.997 * 2.6" | bc
5.1922
```
