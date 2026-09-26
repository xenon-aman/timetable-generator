const API_BASE = "http://127.0.0.1:8080";

function getToken() {
    return localStorage.getItem("token");
}

function setToken(token) {
    localStorage.setItem("token", token);
}

function clearToken() {
    localStorage.removeItem("token");
}

function requireLogin() {
    if (!getToken()) {
        window.location.href = "login.html";
    }
}

async function apiFetch(path, options = {}) {
    const headers = options.headers || {};
    headers["Content-Type"] = "application/json";
    const token = getToken();
    if (token) headers["Authorization"] = "Bearer " + token;

    const res = await fetch(API_BASE + path, { ...options, headers });

    if (res.status === 401) {
        clearToken();
        window.location.href = "login.html";
        throw new Error("Not logged in");
    }

    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || "Request failed");
    return data;
}

function logout() {
    clearToken();
    window.location.href = "login.html";
}