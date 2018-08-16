(fn ack (x y)
    (cond ((= x 0) (+ y 1))
          ((= y 0) (ack (- x 1) 1))
          (t       (ack (- x 1) (ack x (- y 1))))))
