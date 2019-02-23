(defun foo(n)
	(if (zerop n) 0
		(let ()
			(if (evenp n) (print 2) (print 1))
			(foo (- n 1)))))

(foo (read))

