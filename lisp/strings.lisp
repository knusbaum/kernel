(fn reverse (lst)
    (do ((nxt lst (cdr nxt))
         (hd (car nxt) (car nxt))
         (lst2 (cons hd nil) (cons hd lst2)))
        ((eq (cdr nxt) nil) lst2)))


(fn string-to-list (str)
    (let ((len (str-len str))
          (res ()))
      (do ((x 0 (+ x 1)))
          ((or
            (= x len)
            (set res (cons (str-nth str x) res)))
           (reverse res)))))
             
