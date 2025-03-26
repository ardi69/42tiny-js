// test for array contains
var a = [1,2,4,5,7];
var b = ["bread","cheese","sandwich"];

// 42-tiny-js change begin --->
// in JavaScript Array.prototype.contains is not defined
// use Array.prototype.includes instead
//myfoo = eval("{ foo: 42 }");
myfoo = eval("(" + "{ foo: 42 }" + ")");
//<--- 42-tiny-js change end
result = a.includes(1) && !a.includes(42) && b.includes("cheese") && !b.includes("eggs");
