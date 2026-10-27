
const API_BASE = "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev".replace(/\/+$/, "");

const employeeList = document.getElementById("employeeList");
const message = document.getElementById("message");

const firstSearch = document.getElementById("firstSearch");
const lastSearch = document.getElementById("lastSearch");
const locationSearch = document.getElementById("locationSearch");

let employees = [];

function escapeHTML(value) {
  return String(value ?? "").replace(/[&<>"']/g, char => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[char]);
}

function renderEmployees() {
  const first = firstSearch.value.trim().toLowerCase();
  const last = lastSearch.value.trim().toLowerCase();
  const location = locationSearch.value.trim().toLowerCase();

  const filtered = employees.filter(employee =>
    String(employee.firstName ?? "").toLowerCase().includes(first) &&
    String(employee.lastName ?? "").toLowerCase().includes(last) &&
    String(employee.branch ?? "").toLowerCase().includes(location)
  );

  if (filtered.length === 0) {
    employeeList.innerHTML =
      '<tr><td colspan="6">No employees found.</td></tr>';
    return;
  }

  employeeList.innerHTML = filtered.map(employee => `
    <tr>
      <td>${escapeHTML(employee.firstName)} ${escapeHTML(employee.lastName)}</td>
      <td>${escapeHTML(employee.title)}</td>
      <td>${escapeHTML(employee.email)}</td>
      <td>${escapeHTML(employee.phone)}</td>
      <td>${escapeHTML(employee.branch)}</td>
      <td>—</td>
    </tr>
  `).join("");
}

async function loadEmployees() {
  try {
    message.textContent = "Loading staff...";

    const response = await fetch(`${API_BASE}/api/staff`);

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const data = await response.json();

    if (!Array.isArray(data)) {
      throw new Error("API response is not a staff list");
    }

    employees = data;
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
