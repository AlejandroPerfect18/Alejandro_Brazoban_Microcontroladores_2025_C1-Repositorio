function calcularMono() {
    const r1 = parseFloat(document.getElementById("r1_mono").value);
    const c = parseFloat(document.getElementById("c_mono").value) * 1e-9;

    if (isNaN(r1) || isNaN(c)) {
        alert("Por favor ingresa valores válidos.");
        return;
    }

    const t = 1.1 * r1 * c;
    document.getElementById("resultado_mono").textContent = t.toFixed(6);
}

function calcularAstable() {
    const r1 = parseFloat(document.getElementById("r1_astable").value);
    const r2 = parseFloat(document.getElementById("r2_astable").value);
    const c = parseFloat(document.getElementById("c_astable").value) * 1e-9;

    if (isNaN(r1) || isNaN(r2) || isNaN(c)) {
        alert("Por favor ingresa valores válidos.");
        return;
    }

    const t = 0.693 * (r1 + 2 * r2) * c;
    const frecuencia = 1 / t;
    const ciclo = ((r1 + r2) / (r1 + 2 * r2)) * 100;

    document.getElementById("frecuencia").textContent = frecuencia.toFixed(2);
    document.getElementById("ciclo").textContent = ciclo.toFixed(2);
}
