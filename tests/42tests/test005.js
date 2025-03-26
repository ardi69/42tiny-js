// Allgemeiner Test für 42tinyjs (ohne Arrays)
// Getestet werden: Arithmetik, Strings, Objekte, Funktionen, Closures, Rekursion,
// Schleifen, Fehlerbehandlung, JSON-Serialisierung, Generatoren, "this"-Kontext, Logik.

var failedTests = [];

// Hilfsfunktion zum Speichern von Fehlschlägen
function checkTest(testNumber, condition, errorMessage) {
    if (!condition) {
        failedTests.push("Test " + testNumber + (errorMessage ? " (" + errorMessage + ")" : ""));
    }
}

// -----------------------------------------------------------------
// Test 1: Basis-Arithmetik
print("Test 1: Basis-Arithmetik");
try {
    var t1 = (1 + 2 === 3) && (5 - 2 === 3) && (2 * 3 === 6) && (8 / 2 === 4);
    print("Ergebnis Test 1: " + t1);
    checkTest(1, t1);
} catch (e) {
    checkTest(1, false, e);
}

// -----------------------------------------------------------------
// Test 2: String-Operationen
print("Test 2: String-Operationen");
try {
    var str = "Hello, " + "World!";
    var t2 = (str == "Hello, World!" && str.charAt(0) === "H" && str.substring(7, 12) === "World");
    print("Ergebnis Test 2: " + t2);
    checkTest(2, t2);
} catch (e) {
    checkTest(2, false, e);
}

// -----------------------------------------------------------------
// Test 3: Objekt-Manipulation
print("Test 3: Objekt-Manipulation");
try {
    var obj = { a: 10, b: 20 };
    obj.c = obj.a + obj.b;
    var t3 = (obj.c === 30 && "c" in obj);
    print("Ergebnis Test 3: " + t3);
    checkTest(3, t3);
} catch (e) {
    checkTest(3, false, e);
}

// -----------------------------------------------------------------
// Test 4: Funktionen und Closures
print("Test 4: Funktionen und Closures");
try {
    function multiplier(factor) {
        return function(x) { 
            return x * factor; 
        };
    }
    var double = multiplier(2);
    var t4 = (double(5) === 10);
    print("Ergebnis Test 4: " + t4);
    checkTest(4, t4);
} catch (e) {
    checkTest(4, false, e);
}

// -----------------------------------------------------------------
// Test 5: Rekursion (Fakultät)
print("Test 5: Rekursion (Fakultät)");
try {
    function factorial(n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
    var t5 = (factorial(5) === 120);
    print("Ergebnis Test 5: " + t5);
    checkTest(5, t5);
} catch (e) {
    checkTest(5, false, e);
}

// -----------------------------------------------------------------
// Test 6: For-Schleife
print("Test 6: For-Schleife");
try {
    var count = 0;
    for (var i = 0; i < 5; i++) {
        count++;
    }
    var t6 = (count === 5);
    print("Ergebnis Test 6: " + t6);
    checkTest(6, t6);
} catch (e) {
    checkTest(6, false, e);
}

// -----------------------------------------------------------------
// Test 7: While-Schleife
print("Test 7: While-Schleife");
try {
    var countWhile = 0;
    while (countWhile < 5) {
        countWhile++;
    }
    var t7 = (countWhile === 5);
    print("Ergebnis Test 7: " + t7);
    checkTest(7, t7);
} catch (e) {
    checkTest(7, false, e);
}

// -----------------------------------------------------------------
// Test 8: Fehlerbehandlung (try-catch)
print("Test 8: Fehlerbehandlung (try-catch)");
try {
    var errorCaught = false;
    try {
        throw "Test-Fehler";
    } catch (e) {
        errorCaught = (e === "Test-Fehler");
    }
    var t8 = errorCaught;
    print("Ergebnis Test 8: " + t8);
    checkTest(8, t8);
} catch (e) {
    checkTest(8, false, e);
}

// -----------------------------------------------------------------
// Test 9: JSON-Serialisierung
print("Test 9: JSON-Serialisierung");
try {
    var original = { name: "42tinyjs", version: "1.0" };
    var clone = JSON.parse(JSON.stringify(original));
    var t9 = (clone.name === "42tinyjs" && clone.version === "1.0");
    print("Ergebnis Test 9: " + t9);
    checkTest(9, t9);
} catch (e) {
    checkTest(9, false, e);
}

// -----------------------------------------------------------------
// Test 10: Generator-Funktion (Fibonacci)
print("Test 10: Generator-Funktion (Fibonacci)");
try {
    function* fibGenerator() {
        var a = 1, b = 1;
        while (true) {
            yield a;
            var temp = a;
            a = b;
            b = temp + b;
        }
    }
    var gen = fibGenerator();
    var t10 = (gen.next() === 1 && gen.next() === 1 && gen.next() === 2);
    print("Ergebnis Test 10: " + t10);
    checkTest(10, t10);
} catch (e) {
    checkTest(10, false, e);
}

// -----------------------------------------------------------------
// Test 11: "this"-Kontext in Methoden
print("Test 11: 'this'-Kontext");
try {
    var objTest = {
        value: 42,
        getValue: function() {
            return this.value;
        }
    };
    var t11 = (objTest.getValue() === 42);
    print("Ergebnis Test 11: " + t11);
    checkTest(11, t11);
} catch (e) {
    checkTest(11, false, e);
}

// -----------------------------------------------------------------
// Test 12: Logische Operatoren und ternärer Operator
print("Test 12: Logische Operatoren und ternärer Operator");
try {
    var cond = (5 > 3) && (2 < 4);
    var ternary = cond ? "yes" : "no";
    var t12 = (cond === true && ternary === "yes");
    print("Ergebnis Test 12: " + t12);
    checkTest(12, t12);
} catch (e) {
    checkTest(12, false, e);
}

// -----------------------------------------------------------------
// Abschluss der Tests
if (failedTests.length === 0) {
    print("✅ Alle Tests bestanden!");
    result = true;
} else {
    print("❌ Fehler in folgenden Tests:\n" + failedTests.join("\n"));
    result = false;
}
