
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
  return String(value ?? "").replace(/[&<>"']/g, character => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[character]);
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
      <td>
        <button
          type="button"
          class="delete-employee"
          data-id="${escapeHTML(employee.id)}">
          Delete
        </button>
      </td>
    </tr>
  `).join("");
}

async function loadEmployees() {
  try {
    message.textContent = "Loading employees...";

    const response = await fetch(`${API_BASE}/api/staff`);

    if (!response.ok) {
      throw new Error(`Server returned HTTP ${response.status}`);
    }

    const data = await response.json();

    if (!Array.isArray(data)) {
      throw new Error("Unexpected staff data from server");
    }

    employees = data;
    renderEmployees();
    message.textContent = `${employees.length} employees loaded.`;
  } catch (error) {
    console.error("Load employees error:", error);
    message.textContent =
      `Could not load employees: ${error.message}`;
  }
}

// ADD EMPLOYEE
employeeForm.addEventListener("submit", async event => {
  event.preventDefault();

  const submitButton =
    employeeForm.querySelector('button[type="submit"]');

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
      `Employee ${result.firstName ?? ""} ${result.lastName ?? ""} added successfully.`;
  } catch (error) {
    console.error("Add employee error:", error);
    message.textContent =
      `Could not add employee: ${error.message}`;
  } finally {
    submitButton.disabled = false;
  }
});

// DELETE EMPLOYEE
employeeList.addEventListener("click", async event => {
  const button = event.target.closest(".delete-employee");

  if (!button) return;

  const id = button.dataset.id;
  const employee = employees.find(
    item => String(item.id) === String(id)
  );

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

// SEARCH BARS
[firstSearch, lastSearch, locationSearch].forEach(input => {
  input.addEventListener("input", renderEmployees);
});

// INITIAL LOAD
loadEmployees();

const accessPermissionFields = [
  "canViewCustomers",
  "canManageAccounts",
  "canViewReports",
  "canManageStaff",
  "canAddEmployees",
  "canDeleteEmployees"
];

const accessEmployeeSelect = document.getElementById("accessEmployee");
const permissionsForm = document.getElementById("permissionsForm");
const permissionsMessage = document.getElementById("permissionsMessage");

async function loadAccessEmployees() {
  if (!accessEmployeeSelect) return;

  try {
    const response = await fetch(`${API_BASE}/api/staff`);
    if (!response.ok) throw new Error("Employees load nahi huay");

    const employees = await response.json();
    accessEmployeeSelect.innerHTML =
      '<option value="">Select an employee</option>';

    employees.forEach((employee) => {
      const option = document.createElement("option");
      option.value = employee.id;
      option.textContent =
        `${employee.firstName} ${employee.lastName} (${employee.title})`;
      accessEmployeeSelect.appendChild(option);
    });
  } catch (error) {
    console.error(error);
    permissionsMessage.textContent =
      "Employees load nahi huay. Backend server check karo.";
  }
}

if (accessEmployeeSelect) {
  accessEmployeeSelect.addEventListener("change", async () => {
    const employeeId = accessEmployeeSelect.value;

    accessPermissionFields.forEach((field) => {
      document.getElementById(field).checked = false;
    });

    if (!employeeId) {
      permissionsMessage.textContent = "";
      return;
    }

    try {
      const response = await fetch(`${API_BASE}/api/staff`);

      if (!response.ok) throw new Error("Staff data load nahi hua");

      const employees = await response.json();
      const employee = employees.find(
        (item) => String(item.id) === String(employeeId)
      );

      if (!employee) throw new Error("Employee nahi mila");

      accessPermissionFields.forEach((field) => {
        document.getElementById(field).checked =
          employee[field] === true ||
          employee[field] === 1 ||
          employee[field] === "1";
      });

      permissionsMessage.textContent = "Permissions load ho gayi hain.";
    } catch (error) {
      console.error(error);
      permissionsMessage.textContent = "Permissions load nahi ho sakin.";
    }
  });
}

if (permissionsForm) {
  permissionsForm.addEventListener("submit", async (event) => {
    event.preventDefault();

    const employeeId = accessEmployeeSelect.value;

    if (!employeeId) {
      permissionsMessage.textContent = "Pehle employee select karo.";
      return;
    }

    const permissions = {};

    accessPermissionFields.forEach((field) => {
      permissions[field] = document.getElementById(field).checked ? "1" : "0";
    });

    try {
      const response = await fetch(
        `${API_BASE}/api/staff/${encodeURIComponent(employeeId)}/permissions`,
        {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: new URLSearchParams(permissions).toString()
        }
      );

      const result = await response.json();

      if (!response.ok) {
        throw new Error(result.error || "Permissions save nahi huin");
      }

      permissionsMessage.textContent = "Permissions successfully save ho gayi hain.";
    } catch (error) {
      console.error(error);
      permissionsMessage.textContent =
        error.message || "Permissions save nahi huin.";
    }
  });
}

loadAccessEmployees();
