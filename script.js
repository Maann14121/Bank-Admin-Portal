const API_BASE = "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

let employees = [];

const employeeList = document.getElementById("employeeList");
const message = document.getElementById("message");
const firstSearch = document.getElementById("firstSearch");
const lastSearch = document.getElementById("lastSearch");
const locationSearch = document.getElementById("locationSearch");

function escapeHTML(value) {
  return String(value ?? "").replace(/[&<>"']/g, c => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[c]);
}

function renderEmployees() {
  const first = firstSearch.value.trim().toLowerCase();
  const last = lastSearch.value.trim().toLowerCase();
  const location = locationSearch.value.trim().toLowerCase();

  const filtered = employees.filter(e =>
    String(e.firstName ?? "").toLowerCase().includes(first) &&
    String(e.lastName ?? "").toLowerCase().includes(last) &&
    String(e.branch ?? "").toLowerCase().includes(location)
  );

  employeeList.innerHTML = filtered.length
    ? filtered.map(e => `
        <tr>
          <td>${escapeHTML(e.firstName)} ${escapeHTML(e.lastName)}</td>
          <td>${escapeHTML(e.title)}</td>
          <td>${escapeHTML(e.email)}</td>
          <td>${escapeHTML(e.phone)}</td>
          <td>${escapeHTML(e.branch)}</td>
          <td>—</td>
        </tr>
      `).join("")
    : '<tr><td colspan="6">No employees found.</td></tr>';
}

async function loadEmployees() {
  try {
    message.textContent = "Loading staff...";

    const response = await fetch(`${API_BASE}/api/staff`);

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    employees = await response.json();
    renderEmployees();
    message.textContent = `Loaded ${employees.length} employees.`;
  } catch (error) {
    console.error("Staff loading error:", error);
    message.textContent = `Could not load staff: ${error.message}`;
  }
}

[firstSearch, lastSearch, locationSearch].forEach(input => {
  input.addEventListener("input", renderEmployees);
});

loadEmployees();
