// Array-Test für 42tinyjs (isolierte Tests)
// Jeder Test arbeitet mit einem eigenen Array, um Seiteneffekte zu vermeiden.

var failedTests = [];

// Hilfsfunktion zum Speichern von Fehlschlägen
function checkTest(testNumber, condition, errorMessage) {
    if (!condition) {
        failedTests.push("Test " + testNumber + (errorMessage ? " (" + errorMessage + ")" : ""));
    }
}

// -----------------------------------------------------------------
// Test 1: Erstellung und Grundoperationen
print("Test 1: Erstellung und Grundoperationen");
try {
    var arr1 = [10, 20, 30, 40, 50];
    var t1 = (arr1.length === 5 && arr1[0] === 10 && arr1[4] === 50);
    print("Ergebnis Test 1: " + t1);
    checkTest(1, t1);
} catch (e) {
    checkTest(1, false, e);
}

// -----------------------------------------------------------------
// Test 2: Zugriff und Manipulation
print("Test 2: Zugriff und Manipulation");
try {
    var arr2 = [10, 20, 30, 40, 50];
    arr2[2] = 99;
    var t2 = (arr2[2] === 99);
    print("Ergebnis Test 2: " + t2);
    checkTest(2, t2);
} catch (e) {
    checkTest(2, false, e);
}

// -----------------------------------------------------------------
// Test 3: Push() und Pop()
print("Test 3: Push() und Pop()");
try {
    var arr3 = [10, 20, 30, 40, 50];
    arr3.push(60);
    var poppedValue = arr3.pop();
    var t3 = (poppedValue === 60 && arr3.length === 5);
    print("Ergebnis Test 3: " + t3);
    checkTest(3, t3);
} catch (e) {
    checkTest(3, false, e);
}

// -----------------------------------------------------------------
// Test 4: Unshift() und Shift()
print("Test 4: Unshift() und Shift()");
try {
    var arr4 = [10, 20, 30, 40, 50];
    arr4.unshift(5);
    var shiftedValue = arr4.shift();
    var t4 = (shiftedValue === 5 && arr4.length === 5);
    print("Ergebnis Test 4: " + t4);
    checkTest(4, t4);
} catch (e) {
    checkTest(4, false, e);
}

// -----------------------------------------------------------------
// Test 5: Slice() – Teilarray extrahieren
print("Test 5: Slice()");
try {
    var arr5 = [10, 20, 30, 40, 50];
    var sliced = arr5.slice(1, 3);
    // Erwartet: [20, 30]
    var t5 = (sliced.length === 2 && sliced[0] === 20 && sliced[1] === 30);
    print("Ergebnis Test 5: " + t5);
    checkTest(5, t5);
} catch (e) {
    checkTest(5, false, e);
}

// -----------------------------------------------------------------
// Test 6: Splice() – Elemente entfernen
print("Test 6: Splice()");
try {
    var arr6 = [10, 20, 30, 40, 50];
    var spliced = arr6.splice(2, 2); // entfernt 30 und 40
    // arr6 sollte jetzt [10,20,50] sein
    var t6 = (spliced.length === 2 && arr6.length === 3 && arr6[2] === 50);
    print("Ergebnis Test 6: " + t6);
    checkTest(6, t6);
} catch (e) {
    checkTest(6, false, e);
}

// -----------------------------------------------------------------
// Test 7: for-Schleife über ein Array
print("Test 7: for-Schleife über ein Array");
try {
    var arr7 = [10, 20, 30, 40, 50];
    var sum = 0;
    for (var i = 0; i < arr7.length; i++) {
        sum += arr7[i];
    }
    var t7 = (sum === 150);
    print("Ergebnis Test 7: " + t7);
    checkTest(7, t7);
} catch (e) {
    checkTest(7, false, e);
}

// -----------------------------------------------------------------
// Test 8: forEach() Methode
print("Test 8: forEach()");
try {
    var arr8 = [10, 20, 30, 40, 50];
    var sum2 = 0;
    arr8.forEach(function(num) {
        sum2 += num;
    });
    var t8 = (sum2 === 150);
    print("Ergebnis Test 8: " + t8);
    checkTest(8, t8);
} catch (e) {
    checkTest(8, false, e);
}

// -----------------------------------------------------------------
// Test 9: indexOf() und includes()
print("Test 9: indexOf() und includes()");
try {
    var arr9 = [10, 20, 30, 40, 50];
    var t9 = (arr9.indexOf(20) === 1 && arr9.includes(50) && !arr9.includes(99));
    print("Ergebnis Test 9: " + t9);
    checkTest(9, t9);
} catch (e) {
    checkTest(9, false, e);
}

// -----------------------------------------------------------------
// Test 10: Sort()
print("Test 10: Sort()");
try {
    var arr10 = [5, 2, 9, 1, 10];
    arr10.sort(function(a, b) { return a - b; });
    var t10 = (arr10[0] === 1 && arr10[4] === 10);
    print("Ergebnis Test 10: " + t10);
    checkTest(10, t10);
} catch (e) {
    checkTest(10, false, e);
}

// -----------------------------------------------------------------
// Test 11: map() – Transformation von Arrays
print("Test 11: map()");
try {
    var arr11 = [10, 20, 30];
    var squared = arr11.map(function(num) { return num * num; });
    var t11 = (squared[0] === 100 && squared[1] === 400 && squared[2] === 900);
    print("Ergebnis Test 11: " + t11);
    checkTest(11, t11);
} catch (e) {
    checkTest(11, false, e);
}

// -----------------------------------------------------------------
// Test 12: reduce() – Array zu einem Wert reduzieren
print("Test 12: reduce()");
try {
    var arr12 = [10, 20, 30];
    var reducedSum = arr12.reduce(function(acc, num) { return acc + num; }, 0);
    var t12 = (reducedSum === 60);
    print("Ergebnis Test 12: " + t12);
    checkTest(12, t12);
} catch (e) {
    checkTest(12, false, e);
}

// -----------------------------------------------------------------
// Test 13: Mehrdimensionale Arrays
print("Test 13: Mehrdimensionale Arrays");
try {
    var matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
    var t13 = (matrix[1][1] === 5);
    print("Ergebnis Test 13: " + t13);
    checkTest(13, t13);
} catch (e) {
    checkTest(13, false, e);
}

// -----------------------------------------------------------------
// Test 14: Leeres Array prüfen
print("Test 14: Leeres Array");
try {
    var arr14 = [];
    var t14 = (arr14.length === 0);
    print("Ergebnis Test 14: " + t14);
    checkTest(14, t14);
} catch (e) {
    checkTest(14, false, e);
}

// -----------------------------------------------------------------
// Abschluss des Array-Tests
if (failedTests.length === 0) {
    print("✅ Alle Array-Tests bestanden!");
    result = true;
} else {
    print("❌ Fehler in folgenden Tests:\n" + failedTests.join("\n"));
    result = false;
}
