// String-Test für 42tinyjs
// Überprüft verschiedene String-Operationen

var failedTests = [];

// Hilfsfunktion zum Speichern von Fehlschlägen
function checkTest(testNumber, condition, errorMessage) {
    if (!condition) {
        failedTests.push("Test " + testNumber + (errorMessage ? " (" + errorMessage + ")" : ""));
    }
}

// -----------------------------------------------------------------
// Test 1: String-Konkatenation und Grundoperationen
print("Test 1: String-Konkatenation und Grundoperationen");
try {
    var str1 = "Hello, " + "World!";
    var t1 = (str1 === "Hello, World!" && str1.length === 13);
    print("Ergebnis Test 1: " + t1);
    checkTest(1, t1);
} catch (e) {
    checkTest(1, false, e);
}

// -----------------------------------------------------------------
// Test 2: Zeichen-Zugriff mit charAt()
print("Test 2: charAt()");
try {
    var str2 = "JavaScript";
    var t2 = (str2.charAt(0) === "J" && str2.charAt(4) === "S");
    print("Ergebnis Test 2: " + t2);
    checkTest(2, t2);
} catch (e) {
    checkTest(2, false, e);
}

// -----------------------------------------------------------------
// Test 3: substring() und slice()
print("Test 3: substring() und slice()");
try {
    var str3 = "42tinyjs";
    var t3 = (str3.substring(2, 6) === "tiny" && str3.slice(2, 6) === "tiny");
    print("Ergebnis Test 3: " + t3);
    checkTest(3, t3);
} catch (e) {
    checkTest(3, false, e);
}

// -----------------------------------------------------------------
// Test 4: indexOf() und lastIndexOf()
print("Test 4: indexOf() und lastIndexOf()");
try {
    var str4 = "banananana";
    var t4 = (str4.indexOf("na") === 2 && str4.lastIndexOf("na") === 8);
    print("Ergebnis Test 4: " + t4);
    checkTest(4, t4);
} catch (e) {
    checkTest(4, false, e);
}

// -----------------------------------------------------------------
// Test 5: includes(), startsWith(), endsWith()
print("Test 5: includes(), startsWith(), endsWith()");
try {
    var str5 = "Hello, tinyJS!";
    var t5 = (str5.includes("tiny") && str5.startsWith("Hello") && str5.endsWith("!"));
    print("Ergebnis Test 5: " + t5);
    checkTest(5, t5);
} catch (e) {
    checkTest(5, false, e);
}

// -----------------------------------------------------------------
// Test 6: toUpperCase() und toLowerCase()
print("Test 6: toUpperCase() und toLowerCase()");
try {
    var str6 = "MiXeD CaSe";
    var t6 = (str6.toUpperCase() === "MIXED CASE" && str6.toLowerCase() === "mixed case");
    print("Ergebnis Test 6: " + t6);
    checkTest(6, t6);
} catch (e) {
    checkTest(6, false, e);
}

// -----------------------------------------------------------------
// Test 7: trim()
print("Test 7: trim()");
try {
    var str7 = "  whitespace  ";
    var t7 = (str7.trim() === "whitespace");
    print("Ergebnis Test 7: " + t7);
    checkTest(7, t7);
} catch (e) {
    checkTest(7, false, e);
}

// -----------------------------------------------------------------
// Test 8: replace()
print("Test 8: replace()");
try {
    var str8 = "Ich mag Kaffee!";
    var newStr8 = str8.replace("Kaffee", "Tee");
    var t8 = (newStr8 === "Ich mag Tee!");
    print("Ergebnis Test 8: " + t8);
    checkTest(8, t8);
} catch (e) {
    checkTest(8, false, e);
}

// -----------------------------------------------------------------
// Test 9: split() und join()
print("Test 9: split() und join()");
try {
    var str9 = "eins,zwei,drei";
    var parts = str9.split(",");
    var joined = parts.join("-");
    var t9 = (parts.length === 3 && parts[1] === "zwei" && joined === "eins-zwei-drei");
    print("Ergebnis Test 9: " + t9);
    checkTest(9, t9);
} catch (e) {
    checkTest(9, false, e);
}

// -----------------------------------------------------------------
// Test 10: Mehrzeilige Strings mit \n
print("Test 10: Mehrzeilige Strings mit \\n");
try {
    var str10 = "Zeile 1\nZeile 2\nZeile 3";
    var t10 = (str10.split("\n").length === 3);
    print("Ergebnis Test 10: " + t10);
    checkTest(10, t10);
} catch (e) {
    checkTest(10, false, e);
}

// -----------------------------------------------------------------
// Test 11: Escaping von Zeichen
print("Test 11: Escaping von Zeichen");
try {
    var str11 = "Er sagte: \"Hallo!\"";
    var t11 = (str11 === "Er sagte: \"Hallo!\"");
    print("Ergebnis Test 11: " + t11);
    checkTest(11, t11);
} catch (e) {
    checkTest(11, false, e);
}

// -----------------------------------------------------------------
// Abschluss des String-Tests
if (failedTests.length === 0) {
    print("✅ Alle String-Tests bestanden!");
    result = true;
} else {
    print("❌ Fehler in folgenden Tests: " + failedTests.join(", "));
    result = false;
}
