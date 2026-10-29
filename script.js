
const API_BASE =
  "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

// ==========================================
// HTML ELEMENTS
// ==========================================
const employeeList = document.getElementById("employeeList");
const message = document.getElementById("message");
const employeeForm = document.getElementById("employeeForm");

const firstSearch = document.getElementById("firstSearch");
const lastSearch = document.getElementById("lastSearch");
const locationSearch = document.getElementById("locationSearch");

const accessEmployee = document.getElementById("accessEmployee");
const permissionsForm = document.getElementById("permissionsForm");
const permissionsMessage = document.getElementById("permissionsMessage");

const permissionNames = [
  "canViewCustomers",
  "canManageAccounts",
  "canViewReports",
  "canManageStaff",
  "canAddEmployees",
  "canDeleteEmployees"
];

let employees = [];

// ==========================================
// API HELPER
// ==========================================
async function apiRequest(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      ...(options.headers || {})
    }
  });

  const text = await response.text();
  let data = null;

  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      data = { message: text };
    }
  }

  if (!response.ok) {
    throw new Error(
      data?.message || data?.error || `Request failed: HTTP ${response.status}`
    );
  }

  return data;
}

// ==========================================
// MESSAGE HELPERS
// ==========================================
function showMessage(element, text, isError = false) {
  if (!element) return;

  element.textContent = text;
  element.style.color = isError ? "#c62828" : "#218838";
}

// ==========================================
// EMPLOYEE DATA HELPERS
// ==========================================
function employeeFullName(employee) {
  return `${employee.firstName || ""} ${employee.lastName || ""}`.trim();
}

function employeeLocation(employee) {
  return employee.location || employee.branchLocation || "";
}

function escapeHTML(value) {
  return String(value ?? "").replace(/[&<>"']/g, character => {
    const replacements = {
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&#39;"
    };

    return replacements[character];
  });
}

// ==========================================
// LOAD EMPLOYEES
// ==========================================
async function loadEmployees() {
  try {
    const data = await apiRequest("/api/staff");

    employees = Array.isArray(data)
      ? data
      : Array.isArray(data?.staff)
        ? data.staff
        : Array.isArray(data?.employees)
          ? data.employees
          : [];

    renderEmployees();
    renderAccessEmployees();

    showMessage(message, `${employees.length} employee record(s) loaded.`);
    return employees;
  } catch (error) {
    console.error("Could not load employees:", error);
    showMessage(
      message,
      `Employees load nahi huay: ${error.message}`,
      true
    );
    throw error;
  }
}

// ==========================================
// SEARCH EMPLOYEES
// ==========================================
function getFilteredEmployees() {
  const first = (firstSearch?.value || "").trim().toLowerCase();
  const last = (lastSearch?.value || "").trim().toLowerCase();
  const location = (locationSearch?.value || "").trim().toLowerCase();

  return employees.filter(employee => {
    const firstName = (employee.firstName || "").toLowerCase();
    const lastName = (employee.lastName || "").toLowerCase();
    const employeeBranch = employeeLocation(employee).toLowerCase();

    return (
      firstName.includes(first) &&
      lastName.includes(last) &&
      employeeBranch.includes(location)
    );
  });
}

// ==========================================
// DISPLAY EMPLOYEES
// ==========================================
 {
  if (!employeeList) {
    console.warn('Employee list element "#employeeList" nahi mila.');
    return;
  }

  const filteredEmployees = getFilteredEmployees();

  if (filteredEmployees.length === 0) {
    employeeList.innerHTML = "<p>No employees found.</p>";
    return;

function renderEmployees() {
  if (!employeeList) {
    console.warn('Employee list element "#employeeList" nahi mila.');
    return;
  }

  const filteredEmployees = getFilteredEmployees();

  if (filteredEmployees.length === 0) {
    employeeList.innerHTML = `
      <tr>
        <td colspan="6" style="text-align:center; padding:20px;">
          No employees found.
        </td>
      </tr>
    `;
    return;
  }

  employeeList.innerHTML = filteredEmployees.map(employee => {
    const id = escapeHTML(employee.id);
    const name = escapeHTML(employeeFullName(employee));
    const title = escapeHTML(employee.title || employee.jobTitle || "");
    const email = escapeHTML(employee.email || "");
    const contact = escapeHTML(
      employee.phone || employee.contactInfo || employee.contact || ""
    );
    const branch = escapeHTML(
      employee.location || employee.branchLocation || employee.branch || ""
    );

    return `
      <tr>
        <td>${name}</td>
        <td>${title}</td>
        <td>${email}</td>
        <td>${contact}</td>
        <td>${branch}</td>
        <td>
          <button
            type="button"
            data-delete-employee="${id}"
            class="delete-btn"
          >
            Remove
          </button>
        </td>
      </tr>
    `;
  }).join("");
}
// Search bar listeners
[firstSearch, lastSearch, locationSearch].forEach(input => {
  if (input) {
    input.addEventListener("input", renderEmployees);
  }
});

// ==========================================
// ADD EMPLOYEE
// ==========================================
if (employeeForm) {
  employeeForm.addEventListener("submit", async event => {
    event.preventDefault();

    const formValue = id =>
      document.getElementById(id)?.value?.trim() || "";

    const firstName = formValue("firstName");
    const lastName = formValue("lastName");
    const title = formValue("jobTitle") || formValue("title");
    const email = formValue("email");

    const phone =
      formValue("phone") ||
      formValue("contactInfo") ||
      formValue("contact");

    const location =
      formValue("location") ||
      formValue("branchLocation") ||
      formValue("branch");

    if (!firstName || !lastName || !title || !email) {
      showMessage(
        message,
        "First name, last name, title aur email zaroor bharein.",
        true
      );
      return;
    }

    const newEmployee = {
      firstName,
      lastName,
      title,
      email,
      phone,
      location
    };

    const submitButton = employeeForm.querySelector(
      'button[type="submit"], button:not([type])'
    );

    if (submitButton) submitButton.disabled = true;

    try {
      await apiRequest("/api/staff", {
        method: "POST",
        body: JSON.stringify(newEmployee)
      });

      employeeForm.reset();
      await loadEmployees();

      showMessage(message, "Employee successfully add ho gaya.");
    } catch (error) {
      console.error("Add employee failed:", error);
      showMessage(
        message,
        `Employee add nahi hua: ${error.message}`,
        true
      );
    } finally {
      if (submitButton) submitButton.disabled = false;
    }
  });
}

// ==========================================
// DELETE EMPLOYEE
// ==========================================
if (employeeList) {
  employeeList.addEventListener("click", async event => {
    const button = event.target.closest("[data-delete-employee]");
    if (!button) return;

    const id = button.getAttribute("data-delete-employee");
    const employee = employees.find(item => String(item.id) === String(id));

    if (!employee) {
      showMessage(message, "Employee record nahi mila.", true);
      return;
    }

    if (!confirm(`Kya aap ${employeeFullName(employee)} ko remove karna chahte hain?`)) {
      return;
    }

    button.disabled = true;

    try {
      await apiRequest(`/api/staff/${encodeURIComponent(id)}`, {
        method: "DELETE"
      });

      await loadEmployees();
      showMessage(message, "Employee successfully remove ho gaya.");
    } catch (error) {
      console.error("Delete employee failed:", error);
      showMessage(
        message,
        `Employee remove nahi hua: ${error.message}`,
        true
      );
      button.disabled = false;
    }
  });
}

// ==========================================
// STAFF ACCESS DROPDOWN
// ==========================================
function renderAccessEmployees() {
  if (!accessEmployee) return;

  const previousValue = accessEmployee.value;

  accessEmployee.replaceChildren(
    new Option("Select an employee", "")
  );

  employees.forEach(employee => {
    const option = new Option(
      `${employeeFullName(employee)} (${employee.title || employee.jobTitle || "Staff"})`,
      String(employee.id)
    );

    accessEmployee.add(option);
  });

  if (
    previousValue &&
    employees.some(employee => String(employee.id) === previousValue)
  ) {
    accessEmployee.value = previousValue;
  }
}

// ==========================================
// SHOW SELECTED EMPLOYEE PERMISSIONS
// ==========================================
function showEmployeePermissions() {
  if (!accessEmployee) return;

  const selectedId = accessEmployee.value;
  const employee = employees.find(
    item => String(item.id) === String(selectedId)
  );

  permissionNames.forEach(name => {
    const checkbox = document.getElementById(name);

    if (checkbox) {
      checkbox.checked = employee
        ? Boolean(employee[name] ?? employee.permissions?.[name])
        : false;
    }
  });

  if (permissionsMessage) {
    permissionsMessage.textContent = employee
      ? `Permissions for ${employeeFullName(employee)}`
      : "";
  }
}

if (accessEmployee) {
  accessEmployee.addEventListener("change", showEmployeePermissions);
}

// ==========================================
// SAVE PERMISSIONS
// ==========================================
if (permissionsForm) {
  permissionsForm.addEventListener("submit", async event => {
    event.preventDefault();

    const selectedId = accessEmployee?.value;

    if (!selectedId) {
      showMessage(
        permissionsMessage,
        "Pehle employee select karein.",
        true
      );
      return;
    }

    const permissions = {};

    permissionNames.forEach(name => {
      const checkbox = document.getElementById(name);
      permissions[name] = checkbox ? checkbox.checked : false;
    });

    const submitButton = permissionsForm.querySelector(
      'button[type="submit"], button:not([type])'
    );

    if (submitButton) submitButton.disabled = true;

    try {
      await apiRequest(
        `/api/staff/${encodeURIComponent(selectedId)}/permissions`,
        {
          method: "POST",
          body: JSON.stringify(permissions)
        }
      );

      await loadEmployees();

      if (accessEmployee) {
        accessEmployee.value = String(selectedId);
      }

      showEmployeePermissions();

      showMessage(
        permissionsMessage,
        "Permissions successfully save ho gayi."
      );
    } catch (error) {
      console.error("Save permissions failed:", error);
      showMessage(
        permissionsMessage,
        `Permissions save nahi hui: ${error.message}`,
        true
      );
    } finally {
      if (submitButton) submitButton.disabled = false;
    }
  });
}

// ==========================================
// INITIALIZE PORTAL
// ==========================================
async function initializePortal() {
  console.log("Bank Admin Portal: starting...");

  try {
    await loadEmployees();
    renderAccessEmployees();

    console.log("Bank Admin Portal: initialization finished.");
  } catch (error) {
    console.error("Portal initialization failed:", error);
  }
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initializePortal, {
    once: true
  });
} else {
  initializePortal();
}
