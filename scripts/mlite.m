#!/bin/mll -i
(*
 * mLite function library
 * by Nils M Holm, 2014
 * Placed in the public domain.
 *)

fun gcd (a, 0) = a
      | (0, b) = b
      | (a < b, b) = gcd (a, b rem a)
      | (a, b) = gcd (b, a rem b)

fun lcm (a, b) = let val d = gcd (a, b)
                 in a * b div d
                 end

fun ceil x = ~ (floor (~ x))

fun trunc x = (if x < 0 then ceil else floor) x

fun sgn x < 0 = ~1
      | x > 0 =  1
      | _     =  0

fun mod (a, b) = let val r = a rem b
                 in if r = 0 then 0
                    else if sgn a = sgn b then r
                    else b + r
                 end

infix mod = div;

local
  fun pow (x, 0) = 1
      | (x, y rem 2 = 0) = (fn x = x*x) (pow (x, y div 2))
      | (x, y) = x * (fn x = x*x) (pow (x, y div 2))
  and nth_root (n, x) =
        let fun nrt (r, last ~= r) = r
                  | (r, last) = nrt (r + (x / pow (r, n-1) - r) / n, r)
        in nrt (x, x/2+0.1)
        end
  and reduce (d, n) = let val g = gcd (floor d, n)
                      in if d/g > 100 then
                           (false, false)
                         else
                           (floor d div g, n div g)
                      end
  and rational (int d, n) = reduce (d, n)
             | (d, n) = rational (d*10, n*10)
             | (x = 1/3) = (1, 3)
             | (x = 1/6) = (1, 6)
             | (x = 1/7) = (1, 7)
             | (x = 1/9) = (1, 9)
             | x = rational (x, 1)
in
  fun ^ (x, y < 0) = 1 / pow (x, ~y)
      | (x, int y) = pow (x, floor y)
      | (x, y) = let val (n, d) = rational y
                 in if n then
                      pow (nth_root (d, x), n)
                    else
                      error ("no useful approximation for", (x, y))
                 end
  and nthroot (n, x) = nth_root (n, x)
end

infixr ^ > div

fun sqrt x = let fun sqrt2 (r, last = r) = r
                         | (r, last) = sqrt2 ((r + x / r) / 2, r)
             in if x < 0 then
                  error ("sqrt: argument is negative", x)
                else
                  sqrt2 (x, 0)
             end

fun o (f, g) = fn x = f (g x)

infixr o > ^

fun head h :: t = h

fun tail h :: t = t

local
  fun eqvec (a, b, 0) = true
            | (a, b, n) = eq (ref (a, n-1), ref (b, n-1))
                          also eqvec (a, b, n-1)
  and eq ([], []) = true
       | ((), ()) = true
       | (str a, b) = str b also a = b
       | (char a, b) = char b also a = b
       | (real a, b) = real b also a = b
       | (bool a, b) = bool b also a = b
       | (a :: as, b :: bs) = eq (a, b) also eq (as, bs)
       | (a, b) where order a > 1 =
                  order b > 1
                  also let val k = len a
                       in k = len b also eqvec (a, b, k)
                       end 
       | (vec a, b) = vec b
                      also let val k = len a
                           in k = len b also eqvec (a, b, k)
                           end
       | (a, b)   = false
in
  fun eql (a, b) = eq (a, b)
end

infix eql = =

fun iota (a, b) = let fun j (a = b, b, r) = a :: r
                          | (a, b, r) = j (a, b-1, b :: r)
                  in if a > b then
                       error ("iota: bad range", (a, b))
                     else
                       j (a, b, [])
                  end
       | a = iota (1, a)

fun map f = let fun map2 ([], r) = rev r
                       | (x :: xs, r) = map2 (xs, f x :: r)
            in fn a = map2 (a, [])
            end

fun append_map f = 
      let fun map2 [] = []
                 | (x :: xs) = f x @ map2 xs
      in fn a = map2 a
      end

fun foreach f = let fun iter [] = ()
                           | x :: xs = (f x; iter xs)
            in fn a = iter a
            end

fun fold (f, x) = let fun fold2 ([], r) = r
                              | (x :: xs, r) = fold2 (xs, f (r, x))
                  in fn a = fold2 (a, x)
                  end

fun foldr (f, x) = let fun fold2 ([], r) = r
                               | (x :: xs, r) = fold2 (xs, f (x, r))
                   in fn a = fold2 (rev a, x)
                   end

fun filter f = let fun filter2 ([], r) = rev r
                             | (f x :: xs, r) = filter2 (xs, x :: r)
                             | (x :: xs, r) = filter2 (xs, r)
               in fn a = filter2 (a, [])
               end

fun zipwith f = let fun zip (a, [], r) = rev r
                          | ([], b, r) = rev r
                          | (a :: as, b :: bs, r) =
                              zip (as, bs, f (a, b) :: r)
                in fn a b = zip (a, b, [])
                end

val zip = zipwith (fn x = x)

local
  fun fpstr (ds, v) = let val n = len ds
                          and f = conv (ds, 0)
                      in v + f * 10 ^ ~n
                      end
  and conv ([], v) = v
         | (#"." :: ds, v) = fpstr (ds, v)
         | (c_numeric d :: ds, v) = conv (ds, v * 10 + ord d - 48)
         | (d :: _, _) = false
in
  fun ston n = let val d :: ds = explode n
               in if d = #"-" or d = #"~" then
                    ~ ` conv (ds, 0)
                  else
                    conv (d :: ds, 0)
               end
end

local
  fun conv (0, []) = "0"
         | (0, s) = implode s
         | (n, s) = conv (n div 10, chr (n rem 10 + 48) :: s)
  and sconv (x < 0) = "-" @ conv (~x, [])
          | x = conv (x, [])
  and toint (x <> trunc x) = toint (x * 10)
          | x = trunc x
in
  fun ntos (int x) = sconv `floor x
         | x = let val f = conv (toint ` 1 + abs (x - trunc x), [])
                   and i = trunc x
               in sconv i @ "." @ sub (f, 1, len f)
               end
end


(* Base-n conversion, originally by Bruce Axtens, 2014 *)

exception :radix_out_of_range and :unknown_digit;

fun to_radix (0, _, []) = "0"
           | (0, radix, result) = implode result
           | (n, radix > 36, result) = raise :radix_out_of_range
           | (n rem radix > 10, radix, result) =
               to_radix (n div radix, radix,
                         chr (n rem radix + ord #"a" - 10) :: result)
           | (n, radix, result) =
               to_radix (n div radix, radix,
                         chr (n rem radix + ord #"0") :: result)
           | (n, radix) = to_radix (n, radix, [])

fun from_radix (s, radix) =
      let val digits = explode "0123456789abcdefghijklmnopqrstuvwxyz";
          val len_digits = len digits;
          fun index (_, n >= radix, c) = raise :unknown_digit
                  | (h :: t, n, c = h) = n
                  | (_ :: t, n, c) = index (t, n + 1, c)
                  | c = index (digits, 0, c)
          and conv ([], radix, power, n) = n
                 | (h :: t, radix, power, n) =
                     conv (t, radix, power * radix, index h * power + n)
                 | (s, radix) = conv (rev ` explode s, radix, 1, 0)
          in
            conv (s, radix)
          end


(* Triginometric function by Bruce Axtens, 2014 *)

val PI = 3.14159265358979323846264338327950288419716939937510

(* Degrees to Radians *)
fun d2r d = d * PI / 180

(* Radians to Degrees *)
fun r2d r = 180 * r / PI

(* Sine using the function at http://getpocket.com/a/read/373110659 *)
fun sine x = 
      let val a0 = 1.0
          and a1 = ~0.1666666666640169148537065260055
          and a2 =  0.008333333316490113523036717102793
          and a3 = ~0.0001984126600659171392655484413285
          and a4 =  0.000002755690114917374804474016589137
          and a5 = ~0.00000002502845227292692953118686710787
          and a6 =  0.0000000001538730635926417598443354215485
          and x2 = x * x
      in      
          x * (a0 + x2*(a1 + x2*(a2 + x2*(a3 + x2*(a4 + x2*(a5 + x2*a6))))))
      end

(* Cosine defined as an identity based on Sine *)
fun cosine theta = sine (PI / 2 - theta)

(* Tangent theta = Sine theta / Cosine theta *)
fun tangent theta = (sine theta) / (cosine theta)

(* Secant theta = 1 / Cosine theta *)
fun secant theta = 1 / (cosine theta)

(* Cosecant theta = 1 / Sine theta *)
fun cosecant theta = 1 / (sine theta)

(* Cotangent theta = Cosine theta / Sine theta *)
fun cotangent theta = cosine theta / sine theta

(* http://blogs.scientificamerican.com/roots-of-unity/2013/09/12/10-trig-functions-youve-never-heard-of/ *)

(* Versine:             *) fun versin theta = 1 - (cosine theta)
(* Vercosine:           *) fun vercosin theta = 1 + (cosine theta)
(* Coversine:           *) fun coversin theta = 1 - (sine theta)
(* Covercosine:         *) fun covercosine theta = 1 + (sine theta)
(* Haversine:           *) fun haversin theta = (versin theta) / 2
(* Havercosine:         *) fun havercosin theta = (vercosin theta) / 2
(* Hacoversine:         *) fun hacoversin theta = (coversin theta) / 2
(* Hacovercosine:       *) fun hacovercosin theta = (covercosin theta) / 2
(* Exsecant:            *) fun exsec theta = (secant theta) - 1
(* Excosecant:          *) fun excsc theta = (cosecant theta) - 1
