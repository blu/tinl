(defun fib(x y n)
	(print x) (if (zerop n) (print y) (fib y (+ x y) (- n 1))))

(fib 1 1 44)
