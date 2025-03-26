// Template Literal Test für 42tinyjs (inkl. Tagged Templates)

var failedTests = [];

// Hilfsfunktion zum Speichern von Fehlschlägen
function checkTest(testNumber, condition, errorMessage) {
    if (!condition) {
        failedTests.push("Test " + testNumber + (errorMessage ? " (" + errorMessage + ")" : ""));
    }
}

// -----------------------------------------------------------------
// Test 1: Einfache Verwendung von Template Literals
print("Test 1: Einfache Verwendung");
try {
    var str1 = `Hallo Welt`;
    var t1 = (str1 === "Hallo Welt");
    print("Ergebnis Test 1: " + t1);
    checkTest(1, t1);
} catch (e) {
    checkTest(1, false, e);
}

// -----------------------------------------------------------------
// Test 2: Variable Einbindung
print("Test 2: Variable Einbindung");
try {
    var name = "42tinyjs";
    var str2 = `Hallo, ${name}!`;
    var t2 = (str2 === "Hallo, 42tinyjs!");
    print("Ergebnis Test 2: " + t2);
    checkTest(2, t2);
} catch (e) {
    checkTest(2, false, e);
}

// -----------------------------------------------------------------
// Test 3: Tagged Template - Einfache Funktion
print("Test 3: Tagged Template - Einfache Funktion");
try {
    function tag(strings, value) {
        return `Tagged: ${strings[0]}${value}`;
    }
    var str3 = tag`Hallo ${"42tinyjs"}`;
    var t3 = (str3 === "Tagged: Hallo 42tinyjs");
    print("Ergebnis Test 3: " + t3);
    checkTest(3, t3);
} catch (e) {
    checkTest(3, false, e);
}

// -----------------------------------------------------------------
// Test 4: Tagged Template mit mehreren Werten
print("Test 4: Tagged Template mit mehreren Werten");
try {
    function format(strings, ...values) {
        return strings[0] + values.join(", ") + strings[2];
    }
    var str4 = format`Werte: ${10} und ${20}.`;
    var t4 = (str4 === "Werte: 10, 20.");
    print("Ergebnis Test 4: " + t4);
    checkTest(4, t4);
} catch (e) {
    checkTest(4, false, e);
}

// -----------------------------------------------------------------
// Test 5: Tagged Template mit Manipulation
print("Test 5: Tagged Template mit Manipulation");
try {
    function upper(strings, ...values) {
        return strings[0] + values.map(v => v.toUpperCase()).join(" ") + strings[1];
    }
    var str5 = upper`Hallo ${"welt"}!`;
    print(str5)
    var t5 = (str5 === "Hallo WELT!");
    print("Ergebnis Test 5: " + t5);
    checkTest(5, t5);
} catch (e) {
    checkTest(5, false, e);
}

// -----------------------------------------------------------------
// Test 6: Tagged Template mit Berechnung
print("Test 6: Tagged Template mit Berechnung");
try {
    function sum(strings, a, b) {
        return `${strings[0]}${a + b}${strings[2]}`;
    }
    var str6 = sum`Summe: ${5} und ${10}.`;
    var t6 = (str6 === "Summe: 15.");
    print("Ergebnis Test 6: " + t6);
    checkTest(6, t6);
} catch (e) {
    checkTest(6, false, e);
}

// -----------------------------------------------------------------
// Test 7: Mehrzeilige Tagged Templates
print("Test 7: Mehrzeilige Tagged Templates");
try {
    function multiline(strings, ...values) {
        return strings.join("\n") + "\n" + values.join("\n");
    }
    var str7 = multiline`Zeile 1
Zeile 2 ${"Wert"}
Zeile 3`;
    print(str7);
    var t7 = (str7.split("\n").length === 5);
    print("Ergebnis Test 7: " + t7);
    checkTest(7, t7);
} catch (e) {
    checkTest(7, false, e);
}

// -----------------------------------------------------------------
// Abschluss des Template Literal Tests
if (failedTests.length === 0) {
    print("✅ Alle Template Literal Tests bestanden!");
    result = true;
} else {
    print("❌ Fehler in folgenden Tests:\n" + failedTests.join("\n"));
    result = false;
}
