
const API_BASE =
  "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

const employeeList = document.getElementById("employeeList");
const message = document.getElementById("message");
const employeeForm = document.getElementById("employeeForm");

const firstSearch = document.getElementById("firstSearch");
const lastSearch = document.getElementById("lastSearch");
const locationSearch = document.getElementById("locationSearch");

let employees = [];

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
          <td>
            <button
              type="button"
              class="delete-employee"
              data-id="${escapeHTML(e.id)}">
              Delete
            </button>
          </td>
        </tr>
      `).join("")
    : '<tr><td colspan="6">No employees found.</td></tr>';
}

async function loadEmployees() {
  try {
    message.textContent = "Loading staff...";

    const response = await fetch(`${API_BASE}/api/staff`);

    if (!response.ok) {
      throw new Error(`Server returned HTTP ${response.status}`);
    }

    employees = await response.json();
    renderEmployees();
    message.textContent = `Loaded ${employees.length} employees.`;
  } catch (error) {
    console.error("Staff loading error:", error);
    message.textContent =
      `Could not load staff: ${error.message}`;
  }
}

// Add employee
employeeForm.addEventListener("submit", async event => {
  event.preventDefault();

  const submitButton = employeeForm.querySelector(
    'button[type="submit"]'
  );

  const formData = new URLSearchParams({
    firstName: document.getElementById("firstName").value.trim(),
    lastName: document.getElementById("lastName").value.trim(),
    title: document.getElementById("jobTitle").value.trim(),
    email: document.getElementById("email").value.trim(),
    phone: document.getElementById("phone").value.trim(),
    branch: document.getElementById("location").value.trim()
  });

  try {
    submitButton.disabled = true;
    message.textContent = "Saving employee...";

    const response = await fetch(`${API_BASE}/api/staff`, {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body: formData.toString()
    });

    const result = await response.json();

    if (!response.ok) {
      throw new Error(result.error || `HTTP ${response.status}`);
    }

    employeeForm.reset();
    await loadEmployees();
    message.textContent =
      `Employee ${result.firstName} ${result.lastName} added successfully.`;
  } catch (error) {
    console.error("Add employee error:", error);
    message.textContent = `Could not add employee: ${error.message}`;
  } finally {
    submitButton.disabled = false;
  }
});

// Delete employee
employeeList.addEventListener("click", async event => {
  const button = event.target.closest(".delete-employee");

  if (!button) return;

  const id = button.dataset.id;
  const employee = employees.find(e => String(e.id) === String(id));

  if (!employee) return;

  const confirmed = confirm(
    `Kya aap ${employee.firstName} ${employee.lastName} ko delete karna chahte hain?`
  );

  if (!confirmed) return;

  try {
    button.disabled = true;
    message.textContent = "Deleting employee...";

    const response = await fetch(
      `${API_BASE}/api/staff/${encodeURIComponent(id)}`,
      { method: "DELETE" }
    );

    const result = await response.json();

    if (!response.ok) {
      throw new Error(result.error || `HTTP ${response.status}`);
    }

    await loadEmployees();
    message.textContent =
      `${employee.firstName} ${employee.lastName} deleted successfully.`;
  } catch (error) {
    console.error("Delete employee error:", error);
    message.textContent =
      `Could not delete employee: ${error.message}`;
  }
});

// Search filters
[firstSearch, lastSearch, locationSearch].forEach(input => {
  input.addEventListener("input", renderEmployees);
});

// Load initial staff data
loadEmployees();
