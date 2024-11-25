;; this is an in-line comment
(* This is a block comment *)
(*
 * Another block comment
 *)

(*(*(*Nested block comment*)(**)*)*)

val Errors = newvec (1, 0) and Tests = newvec (1, 0)

fun test3 (s, v, x) = (set (Tests, 0, ref (Tests, 0) + 1);
                       if not ` v eql x then
                         (print ` "FAILED: " @ s @ ", got ";
                          println v;
                          set (Errors, 0, ref (Errors, 0) + 1))
                       else
                         ());

fun test4 (s, v, l, h) = (set (Tests, 0, ref (Tests, 0) + 1);
                          if v < l or v > h then
                            (print ` "FAILED: " @ s @ ", got ";
                             println v;
                             set (Errors, 0, ref (Errors, 0) + 1))
                          else
                            ());


(* Literals *)

test3 ("true", true, true);
test3 ("false", false, false);
test3 ("#\"a\"", #"a", #"a");
test3 ("#\"#\"", #"#", #"#");
test3 ("#\"\\\"\"", #"\"", #"\"");
test3 ("#\"backslash\"", #"backslash", chr 92);
test3 ("#\"newline\"", #"newline", chr 10);
test3 ("#\"quote\"", #"quote", chr 34);
test3 ("#\"space\"", #"space", chr 32);
test3 ("#\"tab\"", #"tab", chr 9);
test3 ("\"\"", "", "");
test3 ("\"test\"", "test", "test");
test3 ("\"Test\"", "Test", "Test");
test3 ("\"\\\"hi\\\"\"", "\"hi\"", "\"hi\"");
test3 ("[]", [], []);
test3 ("[1]", [1], [1]);
test3 ("[1,2,3]", [1,2,3], [1,2,3]);
test3 ("[[[1]]]", [[[1]]], [[[1]]]);
test3 ("1 :: [2]", 1 :: [2], [1,2]);
test3 ("1 :: 2 :: [3]", 1 :: 2 :: [3], [1,2,3]);
test3 ("0", 0, 0);
test3 ("123", 123, 123);
test3 ("~1", ~1, ~(1));
test3 ("~456", ~456, ~(456));
test3 ("0.0", 0.0, 0.0);
test3 ("1.0", 1.0, 1.0);
test3 ("0.123", 0.123, 0.123);
test3 ("123.45", 123.45, 123.45);
test3 ("~1.23", ~1.23, ~(1.23));
test3 ("1e5", 1e5, 100000);
test3 ("12.3e4", 12.3e4, 123000);
test3 ("1.23e~2", 1.23e~2, 0.0123);
test3 ("~34e~2", ~34e~2, ~(0.34));
test3 ("()", (), ());
test3 ("(1)", (1), 1);
test3 ("((1))", ((1)), 1);
test3 ("(((1)))", (((1))), 1);
test3 ("(#\"x\")", (#"x"), #"x");
test3 ("(\"x\")", ("x"), "x");
test3 ("(true)", (true), true);
test3 ("([])", ([]), []);
test3 ("([1])", ([1]), [1]);
test3 ("(1,2)", (1,2), (1,2));
test3 ("(1,2,3)", (1,2,3), (1,2,3));
test3 ("#0(1,2)", #0(1,2), 1);
test3 ("#1(1,2)", #1(1,2), 2);
test3 ("(1,(2))", (1,(2)), (1,2));
test3 ("((1),2)", ((1),2), (1,2));
test3 ("((1,2),3)", ((1,2),3), ((1,2),3));
test3 ("(1,(2,3))", (1,(2,3)), (1,(2,3)));

(* Arithmetics *)

test3 ("1234+5678", 1234+5678, 6912);
test3 ("~1234+5678", ~1234+5678, 4444);
test3 ("1234 + ~5678", 1234 + ~5678, ~4444);
test3 ("~1234 + ~5678", ~1234 + ~5678, ~6912);
test3 ("1234+0", 1234+0, 1234);
test3 ("0+1234", 0+1234, 1234);
test3 ("1234-5678", 1234-5678, ~4444);
test3 ("~1234-5678", ~1234-5678, ~6912);
test3 ("1234 - ~5678", 1234 - ~5678, 6912);
test3 ("~1234 - ~5678", ~1234 - ~5678, 4444);
test3 ("1234-0", 1234-0, 1234);
test3 ("0-1234", 0-1234, ~1234);
test3 ("123*456", 123*456, 56088);
test3 ("~123*456", ~123*456, ~56088);
test3 ("123 * ~456", 123 * ~456, ~56088);
test3 ("~123 * ~456", ~123 * ~456, 56088);
test3 ("123 * 1", 123 * 1, 123);
test3 ("1 * 123", 1 * 123, 123);
test4 ("456/123", 456/123, 3.70, 3.71);
test4 ("~456/123", ~456/123, ~3.71, ~3.70);
test4 ("456 / ~123", 456 / ~123, ~3.71, ~3.70);
test4 ("~456 / ~123", ~456 / ~123, 3.70, 3.71);
test3 ("123 / 1", 123 / 1, 123);
test4 ("1 / 123", 1 / 123, 0.008, 0.009);
test3 ("456 div 123", 456 div 123, 3);
test3 ("~456 div 123", ~456 div 123, ~3);
test3 ("456 div ~123", 456 div ~123, ~3);
test3 ("~456 div ~123", ~456 div ~123, 3);
test3 ("123 div 1", 123 div 1, 123);
test3 ("123 div 123", 123 div 123, 1);
test3 ("123 div 124", 123 div 124, 0);
test3 ("456 rem 123", 456 rem 123, 87);
test3 ("~456 rem 123", ~456 rem 123, ~87);
test3 ("456 rem ~123", 456 rem ~123, 87);
test3 ("~456 rem ~123", ~456 rem ~123, ~87);
test3 ("123 rem 1", 123 rem 1, 0);
test3 ("123 rem 123", 123 rem 123, 0);
test3 ("123 rem 124", 123 rem 124, 123);
test3 ("456 mod 123", 456 mod 123, 87);
test3 ("~456 mod 123", ~456 mod 123, 36);
test3 ("456 mod ~123", 456 mod ~123, ~36);
test3 ("~456 mod ~123", ~456 mod ~123, ~87);
test3 ("123 mod 1", 123 mod 1, 0);
test3 ("123 mod 123", 123 mod 123, 0);
test3 ("123 mod 124", 123 mod 124, 123);
test3 ("123^0", 123^0, 1);
test3 ("123^1", 123^1, 123);
test3 ("123^2", 123^2, 15129);
test4 ("123 ^ ~1", 123 ^ ~1, 0.008, 0.009);
test4 ("123 ^ ~2", 123 ^ ~2, 6.6e~5, 6.7e~5);
test4 ("123 ^ (1/2)", 123 ^ (1/2), 11.09, 11.1);
test4 ("123 ^ (1/3)", 123 ^ (1/3), 4.973, 4.974);
test3 ("256 ^ (1/8)", 256 ^ (1/8), 2.0);
test3 ("(~ o abs) 1", (~ o abs) 1, ~1);
test3 ("(~ o abs) ~1", (~ o abs) ~1, ~1);
test3 ("(abs o ~) 1", (abs o ~) 1, 1);
test3 ("(abs o ~) ~1", (abs o ~) ~1, 1);
test3 ("0 = 0", 0 = 0, true);
test3 ("1 = 1", 1 = 1, true);
test3 ("123 = 123", 123 = 123, true);
test3 ("~123 = ~123", ~123 = ~123, true);
test3 ("0.0 = 0.0", 0.0 = 0.0, true);
test3 ("0 = 1", 0 = 1, false);
test3 ("1 = 0", 1 = 0, false);
test3 ("123 = 123.5", 123 = 123.5, false);
test3 ("123.5 = 123", 123.5 = 123, false);
test3 ("123 = ~123", 123 = ~123, false);
test3 ("~123 = 123", ~123 = 123, false);
test3 ("0 <> 1", 0 <> 1, true);
test3 ("1 <> 0", 1 <> 0, true);
test3 ("123 <> 123.5", 123 <> 123.5, true);
test3 ("123.5 <> 123", 123.5 <> 123, true);
test3 ("123 <> ~123", 123 <> ~123, true);
test3 ("~123 <> 123", ~123 <> 123, true);
test3 ("0 <> 0", 0 <> 0, false);
test3 ("1 <> 1", 1 <> 1, false);
test3 ("123 <> 123", 123 <> 123, false);
test3 ("~123 <> ~123", ~123 <> ~123, false);
test3 ("0.0 <> 0.0", 0.0 <> 0.0, false);
test3 ("~1 < 0", ~1 < 0, true);
test3 ("0 < 1", 0 < 1, true);
test3 ("1 < 2", 1 < 2, true);
test3 ("123 < 123.5", 123 < 123.5, true);
test3 ("122.5 < 123", 122.5 < 123, true);
test3 ("~123 < 123", ~123 < 123, true);
test3 ("~123.5 < 123.5", ~123.5 < 123.5, true);
test3 ("0 < 0", 0 < 0, false);
test3 ("123 < 123", 123 < 123, false);
test3 ("~123 < ~123", ~123 < ~123, false);
test3 ("123.5 < 123.5", 123.5 < 123.5, false);
test3 ("1 < 0", 1 < 0, false);
test3 ("124 < 123", 124 < 123, false);
test3 ("123.5 < 123", 123.5 < 123, false);
test3 ("0 > ~1", 0 > ~1, true);
test3 ("1 > 0", 1 > 0, true);
test3 ("2 > 1", 2 > 1, true);
test3 ("123.5 > 123", 123.5 > 123, true);
test3 ("123 > 122.5", 123 > 122.5, true);
test3 ("123 > ~123", 123 > ~123, true);
test3 ("123.5 > ~123.5", 123.5 > ~123.5, true);
test3 ("0 > 0", 0 > 0, false);
test3 ("123 > 123", 123 > 123, false);
test3 ("~123 > ~123", ~123 > ~123, false);
test3 ("123.5 > 123.5", 123.5 > 123.5, false);
test3 ("0 > 1", 0 > 1, false);
test3 ("123 > 124", 123 > 124, false);
test3 ("123 > 123.5", 123 > 123.5, false);
test3 ("~1 <= 0", ~1 <= 0, true);
test3 ("0 <= 1", 0 <= 1, true);
test3 ("1 <= 2", 1 <= 2, true);
test3 ("123 <= 123.5", 123 <= 123.5, true);
test3 ("122.5 <= 123", 122.5 <= 123, true);
test3 ("~123 <= 123", ~123 <= 123, true);
test3 ("~123.5 <= 123.5", ~123.5 <= 123.5, true);
test3 ("0 <= 0", 0 <= 0, true);
test3 ("123 <= 123", 123 <= 123, true);
test3 ("123.5 <= 123.5", 123.5 <= 123.5, true);
test3 ("~123 <= ~123", ~123 <= ~123, true);
test3 ("1 <= 0", 1 <= 0, false);
test3 ("124 <= 123", 124 <= 123, false);
test3 ("123.5 <= 123", 123.5 <= 123, false);
test3 ("0 >= ~1", 0 >= ~1, true);
test3 ("1 >= 0", 1 >= 0, true);
test3 ("2 >= 1", 2 >= 1, true);
test3 ("123.5 >= 123", 123.5 >= 123, true);
test3 ("123 >= 122.5", 123 >= 122.5, true);
test3 ("123 >= ~123", 123 >= ~123, true);
test3 ("123.5 >= ~123.5", 123.5 >= ~123.5, true);
test3 ("0 >= 0", 0 >= 0, true);
test3 ("123 >= 123", 123 >= 123, true);
test3 ("~123 >= ~123", ~123 >= ~123, true);
test3 ("123.5 >= 123.5", 123.5 >= 123.5, true);
test3 ("0 >= 1", 0 >= 1, false);
test3 ("123 >= 124", 123 >= 124, false);
test3 ("123 >= 123.5", 123 >= 123.5, false);

(* Char/String comparison *)

test3 ("#\"x\" = #\"x\"", #"x" = #"x", true);
test3 ("#\"x\" = #\"y\"", #"x" = #"y", false);
test3 ("#\"X\" = #\"x\"", #"X" = #"x", false);
test3 ("#\"x\" ~= #\"x\"", #"x" ~= #"x", true);
test3 ("#\"X\" ~= #\"x\"", #"X" ~= #"x", true);
test3 ("#\"x\" ~= #\"y\"", #"x" ~= #"y", false);
test3 ("#\"x\" <> #\"x\"", #"x" <> #"x", false);
test3 ("#\"x\" <> #\"y\"", #"x" <> #"y", true);
test3 ("#\"X\" <> #\"x\"", #"X" <> #"x", true);
test3 ("#\"x\" ~<> #\"x\"", #"x" ~<> #"x", false);
test3 ("#\"X\" ~<> #\"x\"", #"X" ~<> #"x", false);
test3 ("#\"x\" ~<> #\"y\"", #"x" ~<> #"y", true);
test3 ("#\"x\" < #\"y\"", #"x" < #"y", true);
test3 ("#\"y\" < #\"x\"", #"y" < #"x", false);
test3 ("#\"x\" < #\"Y\"", #"x" < #"Y", false);
test3 ("#\"x\" < #\"x\"", #"x" < #"x", false);
test3 ("#\"x\" ~< #\"y\"", #"x" ~< #"y", true);
test3 ("#\"x\" ~< #\"Y\"", #"x" ~< #"Y", true);
test3 ("#\"y\" ~< #\"x\"", #"y" ~< #"x", false);
test3 ("#\"x\" ~< #\"X\"", #"x" ~< #"X", false);
test3 ("#\"y\" > #\"x\"", #"y" > #"x", true);
test3 ("#\"x\" > #\"y\"", #"x" > #"y", false);
test3 ("#\"Y\" > #\"x\"", #"Y" > #"x", false);
test3 ("#\"x\" > #\"x\"", #"x" > #"x", false);
test3 ("#\"y\" ~> #\"x\"", #"y" ~> #"x", true);
test3 ("#\"Y\" ~> #\"x\"", #"Y" ~> #"x", true);
test3 ("#\"x\" ~> #\"y\"", #"x" ~> #"y", false);
test3 ("#\"X\" ~> #\"x\"", #"X" ~> #"x", false);
test3 ("#\"x\" <= #\"y\"", #"x" <= #"y", true);
test3 ("#\"y\" <= #\"x\"", #"y" <= #"x", false);
test3 ("#\"x\" <= #\"Y\"", #"x" <= #"Y", false);
test3 ("#\"x\" <= #\"x\"", #"x" <= #"x", true);
test3 ("#\"x\" ~<= #\"y\"", #"x" ~<= #"y", true);
test3 ("#\"x\" ~<= #\"Y\"", #"x" ~<= #"Y", true);
test3 ("#\"y\" ~<= #\"x\"", #"y" ~<= #"x", false);
test3 ("#\"x\" ~<= #\"X\"", #"x" ~<= #"X", true);
test3 ("#\"y\" >= #\"x\"", #"y" >= #"x", true);
test3 ("#\"x\" >= #\"y\"", #"x" >= #"y", false);
test3 ("#\"Y\" >= #\"x\"", #"Y" >= #"x", false);
test3 ("#\"x\" >= #\"x\"", #"x" >= #"x", true);
test3 ("#\"y\" ~>= #\"x\"", #"y" ~>= #"x", true);
test3 ("#\"Y\" ~>= #\"x\"", #"Y" ~>= #"x", true);
test3 ("#\"x\" ~>= #\"y\"", #"x" ~>= #"y", false);
test3 ("#\"X\" ~>= #\"x\"", #"X" ~>= #"x", true);
test3 ("\"test\" < \"test\"", "test" < "test", false);
test3 ("\"test\" < \"tesa\"", "test" < "tesa", false);
test3 ("\"test\" < \"tesz\"", "test" < "tesz", true);
test3 ("\"TEST\" < \"tesa\"", "TEST" < "tesa", true);
test3 ("\"TEST\" < \"tesz\"", "TEST" < "tesz", true);
test3 ("\"test\" < \"TESA\"", "test" < "TESA", false);
test3 ("\"test\" < \"TESZ\"", "test" < "TESZ", false);
test3 ("\"TEST\" < \"TESA\"", "TEST" < "TESA", false);
test3 ("\"TEST\" < \"TESZ\"", "TEST" < "TESZ", true);
test3 ("\"tes\" < \"test\"", "tes" < "test", true);
test3 ("\"test\" < \"tes\"", "test" < "tes", false);
test3 ("\"test\" < \"test0\"", "test" < "test0", true);
test3 ("\"test0\" < \"test\"", "test0" < "test", false);
test3 ("\"test\" <= \"test\"", "test" <= "test", true);
test3 ("\"test\" <= \"tesa\"", "test" <= "tesa", false);
test3 ("\"test\" <= \"tesz\"", "test" <= "tesz", true);
test3 ("\"TEST\" <= \"tesa\"", "TEST" <= "tesa", true);
test3 ("\"TEST\" <= \"tesz\"", "TEST" <= "tesz", true);
test3 ("\"test\" <= \"TESA\"", "test" <= "TESA", false);
test3 ("\"test\" <= \"TESZ\"", "test" <= "TESZ", false);
test3 ("\"TEST\" <= \"TESA\"", "TEST" <= "TESA", false);
test3 ("\"TEST\" <= \"TESZ\"", "TEST" <= "TESZ", true);
test3 ("\"test\" <= \"tes\"", "test" <= "tes", false);
test3 ("\"test\" <= \"test0\"", "test" <= "test0", true);
test3 ("\"test0\" <= \"test\"", "test0" <= "test", false);
test3 ("\"test\" ~< \"test\"", "test" ~< "test", false);
test3 ("\"test\" ~< \"tesa\"", "test" ~< "tesa", false);
test3 ("\"test\" ~< \"tesz\"", "test" ~< "tesz", true);
test3 ("\"TEST\" ~< \"tesa\"", "TEST" ~< "tesa", false);
test3 ("\"TEST\" ~< \"tesz\"", "TEST" ~< "tesz", true);
test3 ("\"test\" ~< \"TESA\"", "test" ~< "TESA", false);
test3 ("\"test\" ~< \"TESZ\"", "test" ~< "TESZ", true);
test3 ("\"TEST\" ~< \"TESA\"", "TEST" ~< "TESA", false);
test3 ("\"TEST\" ~< \"TESZ\"", "TEST" ~< "TESZ", true);
test3 ("\"test\" ~< \"tes\"", "test" ~< "tes", false);
test3 ("\"tes\" ~< \"test\"", "tes" ~< "test", true);
test3 ("\"test\" ~< \"test0\"", "test" ~< "test0", true);
test3 ("\"test0\" ~< \"test\"", "test0" ~< "test", false);
test3 ("\"test\" ~<= \"test\"", "test" ~<= "test", true);
test3 ("\"test\" ~<= \"tesa\"", "test" ~<= "tesa", false);
test3 ("\"test\" ~<= \"tesz\"", "test" ~<= "tesz", true);
test3 ("\"TEST\" ~<= \"tesa\"", "TEST" ~<= "tesa", false);
test3 ("\"TEST\" ~<= \"tesz\"", "TEST" ~<= "tesz", true);
test3 ("\"test\" ~<= \"TESA\"", "test" ~<= "TESA", false);
test3 ("\"test\" ~<= \"TESZ\"", "test" ~<= "TESZ", true);
test3 ("\"TEST\" ~<= \"TESA\"", "TEST" ~<= "TESA", false);
test3 ("\"TEST\" ~<= \"TESZ\"", "TEST" ~<= "TESZ", true);
test3 ("\"tes\" ~<= \"test\"", "tes" ~<= "test", true);
test3 ("\"test\" ~<= \"tes\"", "test" ~<= "tes", false);
test3 ("\"test\" ~<= \"test0\"", "test" ~<= "test0", true);
test3 ("\"test0\" ~<= \"test\"", "test0" ~<= "test", false);
test3 ("\"test\" > \"test\"", "test" > "test", false);
test3 ("\"test\" > \"tesa\"", "test" > "tesa", true);
test3 ("\"test\" > \"tesz\"", "test" > "tesz", false);
test3 ("\"TEST\" > \"tesa\"", "TEST" > "tesa", false);
test3 ("\"TEST\" > \"tesz\"", "TEST" > "tesz", false);
test3 ("\"test\" > \"TESA\"", "test" > "TESA", true);
test3 ("\"test\" > \"TESZ\"", "test" > "TESZ", true);
test3 ("\"TEST\" > \"TESA\"", "TEST" > "TESA", true);
test3 ("\"TEST\" > \"TESZ\"", "TEST" > "TESZ", false);
test3 ("\"tes\" > \"test\"", "tes" > "test", false);
test3 ("\"test\" > \"tes\"", "test" > "tes", true);
test3 ("\"test\" > \"test0\"", "test" > "test0", false);
test3 ("\"test0\" > \"test\"", "test0" > "test", true);
test3 ("\"test\" >= \"test\"", "test" >= "test", true);
test3 ("\"test\" >= \"tesa\"", "test" >= "tesa", true);
test3 ("\"test\" >= \"tesz\"", "test" >= "tesz", false);
test3 ("\"TEST\" >= \"tesa\"", "TEST" >= "tesa", false);
test3 ("\"TEST\" >= \"tesz\"", "TEST" >= "tesz", false);
test3 ("\"test\" >= \"TESA\"", "test" >= "TESA", true);
test3 ("\"test\" >= \"TESZ\"", "test" >= "TESZ", true);
test3 ("\"TEST\" >= \"TESA\"", "TEST" >= "TESA", true);
test3 ("\"TEST\" >= \"TESZ\"", "TEST" >= "TESZ", false);
test3 ("\"tes\" >= \"test\"", "tes" >= "test", false);
test3 ("\"test\" >= \"tes\"", "test" >= "tes", true);
test3 ("\"test\" >= \"test0\"", "test" >= "test0", false);
test3 ("\"test0\" >= \"test\"", "test0" >= "test", true);
test3 ("\"test\" ~> \"test\"", "test" ~> "test", false);
test3 ("\"test\" ~> \"tesa\"", "test" ~> "tesa", true);
test3 ("\"test\" ~> \"tesz\"", "test" ~> "tesz", false);
test3 ("\"TEST\" ~> \"tesa\"", "TEST" ~> "tesa", true);
test3 ("\"TEST\" ~> \"tesz\"", "TEST" ~> "tesz", false);
test3 ("\"test\" ~> \"TESA\"", "test" ~> "TESA", true);
test3 ("\"test\" ~> \"TESZ\"", "test" ~> "TESZ", false);
test3 ("\"TEST\" ~> \"TESA\"", "TEST" ~> "TESA", true);
test3 ("\"TEST\" ~> \"TESZ\"", "TEST" ~> "TESZ", false);
test3 ("\"tes\" ~> \"test\"", "tes" ~> "test", false);
test3 ("\"test\" ~> \"tes\"", "test" ~> "tes", true);
test3 ("\"test\" ~> \"test0\"", "test" ~> "test0", false);
test3 ("\"test0\" ~> \"test\"", "test0" ~> "test", true);
test3 ("\"test\" ~>= \"test\"", "test" ~>= "test", true);
test3 ("\"test\" ~>= \"tesa\"", "test" ~>= "tesa", true);
test3 ("\"test\" ~>= \"tesz\"", "test" ~>= "tesz", false);
test3 ("\"TEST\" ~>= \"tesa\"", "TEST" ~>= "tesa", true);
test3 ("\"TEST\" ~>= \"tesz\"", "TEST" ~>= "tesz", false);
test3 ("\"test\" ~>= \"TESA\"", "test" ~>= "TESA", true);
test3 ("\"test\" ~>= \"TESZ\"", "test" ~>= "TESZ", false);
test3 ("\"TEST\" ~>= \"TESA\"", "TEST" ~>= "TESA", true);
test3 ("\"TEST\" ~>= \"TESZ\"", "TEST" ~>= "TESZ", false);
test3 ("\"tes\" ~>= \"test\"", "tes" ~>= "test", false);
test3 ("\"test\" ~>= \"tes\"", "test" ~>= "tes", true);
test3 ("\"test\" ~>= \"test0\"", "test" ~>= "test0", false);
test3 ("\"test0\" ~>= \"test\"", "test0" ~>= "test", true);
test3 ("\"abc\" = \"abc\"", "abc" = "abc", true);
test3 ("\"aBc\" = \"abc\"", "aBc" = "abc", false);
test3 ("\"abc\" = \"abd\"", "abc" = "abd", false);
test3 ("\"abc\" = \"abcd\"", "abc" = "abcd", false);
test3 ("\"abcd\" = \"abc\"", "abcd" = "abc", false);
test3 ("\"abc\" ~= \"abc\"", "abc" ~= "abc", true);
test3 ("\"abC\" ~= \"abc\"", "abC" ~= "abc", true);
test3 ("\"aBc\" ~= \"abc\"", "aBc" ~= "abc", true);
test3 ("\"aBC\" ~= \"abc\"", "aBC" ~= "abc", true);
test3 ("\"Abc\" ~= \"abc\"", "Abc" ~= "abc", true);
test3 ("\"AbC\" ~= \"abc\"", "AbC" ~= "abc", true);
test3 ("\"ABc\" ~= \"abc\"", "ABc" ~= "abc", true);
test3 ("\"ABC\" ~= \"abc\"", "ABC" ~= "abc", true);
test3 ("\"aBc\" ~= \"AbC\"", "aBc" ~= "AbC", true);
test3 ("\"abc\" ~= \"abd\"", "abc" ~= "abd", false);
test3 ("\"abc\" ~= \"abcd\"", "abc" ~= "abcd", false);
test3 ("\"abcd\" ~= \"abc\"", "abcd" ~= "abc", false);
test3 ("\"abc\" = \"abc\"", "abc" = "abc", true);
test3 ("\"aBc\" = \"abc\"", "aBc" = "abc", false);
test3 ("\"abc\" = \"abd\"", "abc" = "abd", false);
test3 ("\"abc\" = \"abcd\"", "abc" = "abcd", false);
test3 ("\"abcd\" = \"abc\"", "abcd" = "abc", false);
test3 ("\"abc\" <> \"abc\"", "abc" <> "abc", false);
test3 ("\"aBc\" <> \"abc\"", "aBc" <> "abc", true);
test3 ("\"abc\" <> \"abd\"", "abc" <> "abd", true);
test3 ("\"abc\" <> \"abcd\"", "abc" <> "abcd", true);
test3 ("\"abcd\" <> \"abc\"", "abcd" <> "abc", true);
test3 ("\"abc\" ~= \"abc\"", "abc" ~= "abc", true);
test3 ("\"abC\" ~= \"abc\"", "abC" ~= "abc", true);
test3 ("\"aBc\" ~= \"abc\"", "aBc" ~= "abc", true);
test3 ("\"aBC\" ~= \"abc\"", "aBC" ~= "abc", true);
test3 ("\"Abc\" ~= \"abc\"", "Abc" ~= "abc", true);
test3 ("\"AbC\" ~= \"abc\"", "AbC" ~= "abc", true);
test3 ("\"ABc\" ~= \"abc\"", "ABc" ~= "abc", true);
test3 ("\"ABC\" ~= \"abc\"", "ABC" ~= "abc", true);
test3 ("\"aBc\" ~= \"AbC\"", "aBc" ~= "AbC", true);
test3 ("\"abc\" ~= \"abd\"", "abc" ~= "abd", false);
test3 ("\"abc\" ~= \"abcd\"", "abc" ~= "abcd", false);
test3 ("\"abcd\" ~= \"abc\"", "abcd" ~= "abc", false);
test3 ("\"abc\" ~<> \"abc\"", "abc" ~<> "abc", false);
test3 ("\"abC\" ~<> \"abc\"", "abC" ~<> "abc", false);
test3 ("\"aBc\" ~<> \"abc\"", "aBc" ~<> "abc", false);
test3 ("\"aBC\" ~<> \"abc\"", "aBC" ~<> "abc", false);
test3 ("\"Abc\" ~<> \"abc\"", "Abc" ~<> "abc", false);
test3 ("\"AbC\" ~<> \"abc\"", "AbC" ~<> "abc", false);
test3 ("\"ABc\" ~<> \"abc\"", "ABc" ~<> "abc", false);
test3 ("\"ABC\" ~<> \"abc\"", "ABC" ~<> "abc", false);
test3 ("\"aBc\" ~<> \"AbC\"", "aBc" ~<> "AbC", false);
test3 ("\"abc\" ~<> \"abd\"", "abc" ~<> "abd", true);
test3 ("\"abc\" ~<> \"abcd\"", "abc" ~<> "abcd", true);
test3 ("\"abcd\" ~<> \"abc\"", "abcd" ~<> "abc", true);

(* Structual Operations *)

test3 ("1 :: []", 1 :: [], [1]);
test3 ("[1] :: []", [1] :: [], [[1]]);
test3 ("1 :: [2]", 1 :: [2], [1,2]);
test3 ("1 :: 2 :: []", 1 :: 2 :: [], [1,2]);
test3 ("1 :: 2 :: [3]", 1 :: 2 :: [3], [1,2,3]);
test3 ("[1] :: [2]", [1] :: [2], [[1],2]);
test3 ("[] @ []", [] @ [], []);
test3 ("[1,2,3] @ []", [1,2,3] @ [], [1,2,3]);
test3 ("[] @ [1,2,3]", [] @ [1,2,3], [1,2,3]);
test3 ("[1] @ [2]", [1] @ [2], [1,2]);
test3 ("[1,2] @ [3,4]", [1,2] @ [3,4], [1,2,3,4]);
test3 ("\"\" @ \"\"", "" @ "", "");
test3 ("\"abc\" @ \"\"", "abc" @ "", "abc");
test3 ("\"\" @ \"abc\"", "" @ "abc", "abc");
test3 ("\"a\" @ \"b\"", "a" @ "b", "ab");
test3 ("\"ab\" @ \"cd\"", "ab" @ "cd", "abcd");
(*------------------------------------------------------------
fn application operators infix infixr nonfix let
; also or handle raise if case << >>
val fun exception type local
~= ~<> (real)
~ abs ceil floor gcd lcm max min sgn trunc sqrt
append_map clone eql explode filter fold foldr foreach
head implode iota len map newstr newvec order ref rev
set setvec sub tail zip zipwith
bool char chr int not ntos ord real ston str vec
c_alphabetic c_downcase c_lower c_numeric c_upcase c_upper
c_whitespace
append_stream close eof instream load outstream peekc
print println readc readln
------------------------------------------------------------*)

(* Miscellanea *)

fun P (x, y)    = P (x, y, 1)
    | (x, 0, r) = r
    | (x, y, r) = P (x, y-1, x*r)
    ;

test3 ("P (2, 100)", P (2, 100), 1267650600228229401496703205376);

val evenodd = let
  fun e x = if x=0 then true else o (x-1)
  and o x = if x=0 then false else e (x-1)
in
  [e 5, o 5]
end;

test3 ("evenodd", evenodd, [false, true]);

val evenodd_2 =
  let fun e 0 = true
          | x = o (x-1)
      and o 0 = false
          | x = e (x-1)
  in
    [e 5, o 5]
  end;

test3 ("evenodd_2", evenodd_2, [false, true]);

type :list = :nil | :cons(x, :list)

fun length :nil         = 0
         | :cons (_, t) = 1 + length t

local
  fun j (0, r) = r
      | (x, r) = j (x-1, :cons(x, r))
in
  fun numlist x = j (x, :nil)
end;

test3 ("length ` numlist 100", length ` numlist 100, 100);

type :L = :N | :c (x, :L)

infixr :c = @

fun ll :N         = 0
     | (x=1) :c t = 1 + ll t
     | _ :c t     = ll t
     ;
test3 ("ll ` 1 :c 0 :c 1 :c 0 :c 1 :c :N", ll ` 1 :c 0 :c 1 :c 0 :c 1 :c :N, 3);

fun j (0, r) = r
    | (x, r) = j (x-1, x :: r)
    | x = j (x, []);

test3 ("len ` j 1000", len ` j 1000, 1000);

local
  fun map' (f, [])     = []
         | (f, h :: t) = f h :: map' (f, t)
in
  fun map1 f = fn x = map' (f, x)
end;

test3 ("map1 ~ [1,2,3]", map1 ~ [1,2,3], [~1,~2,~3]);

fun map f = let
              fun map' []     = []
                     | h :: t = f h :: map' t
            in
              fn x = map' x
            end;

test3 ("map ~ [1,2,3]", map ~ [1,2,3], [~1,~2,~3]);

exception :exp

fun f x = if x then 0 else raise :exp

test3 ("f false handle :foo = 1 | :exp = 2 | :bar = 3", f false handle :foo = 1 | :exp = 2 | :bar = 3, 2);

exception :overflow

fun change (_, 0)  = []
         | ([], _) = raise :overflow
         | (coin :: coins, amt) =
             if coin > amt then
               change (coins, amt)
             else
               coin :: change (coin :: coins, amt - coin)
               handle :overflow = change (coins, amt);

test3 ("change ([5,2], 16)", change ([5,2], 16), [5,5,2,2,2]);

type :tree = :leaf (x) | :node (:tree, :tree)

fun depth :leaf _      = 1
        | :node (l, r) = 1 + max (depth l, depth r)

val tree = :node (:node (:node (:leaf 1, :leaf 2),
                         :node (:leaf 3, :leaf 4)),
                 (:node (:leaf 3, :leaf 4)));

test3 ("depth tree", depth tree, 4);

println ("Passed "
         @ (ntos ` ref (Tests, 0) - ref (Errors, 0))
         @ " of "
         @ (ntos ` ref (Tests, 0))
         @ " tests")
if ref (Errors, 0) > 0 then
   println (ntos (ref (Errors, 0)) @ " ERROR(S)")
else
   println "Everything fine!"

