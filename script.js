
"use strict";

// ==========================================
// BANK ADMIN PORTAL - COMPLETE FRONTEND
// ==========================================

const API_BASE =
  "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

// Global employee data
let employees = [];

// Permission checkbox IDs
const PERMISSION_IDS = [
  "canViewCustomers",
  "canManageAccounts",
  "canViewReports",
  "canManageStaff",
  "canAddEmployees",
  "canDeleteEmployees"
];

// ==========================================
// HTML ELEMENT HELPERS
// ==========================================

function getElement(id) {
  return document.getElementById(id);
}

function getValue(id) {
  const element = getElement(id);
  return element ? element.value.trim() : "";
}

function showMessage(text, isError = false) {
  const message = getElement("message");

  if (message) {
    message.textContent = text;
    message.style.color = isError ? "#dc2626" : "#15803d";
  } else {
    console.log(text);
  }
}

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
  let data = {};

  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      data = { message: text };
    }
  }

  if (!response.ok) {
    throw new Error(
      data.message ||
      data.error ||
      `Request failed: HTTP ${response.status}`
    );
  }

  return data;
}

// ==========================================
// NORMALIZE EMPLOYEE DATA
// Supports common backend field names
// ==========================================

function normalizeEmployee(employee, index) {
  const firstName =
    employee.firstName ??
    employee.first_name ??
    "";

  const lastName =
    employee.lastName ??
    employee.last_name ??
    "";

  const fullName =
    employee.name ??
    employee.fullName ??
    employee.full_name ??
    `${firstName} ${lastName}`.trim();

  const nameParts = String(fullName).trim().split(/\s+/);

  return {
    ...employee,

    id: employee.id ??
        employee.staffId ??
        employee.staff_id ??
        employee.employeeId ??
        employee.employee_id ??
        index,

    firstName: firstName || nameParts[0] || "",
    lastName: lastName || nameParts.slice(1).join(" "),
    name: fullName || "Unnamed Employee",

    jobTitle:
      employee.jobTitle ??
      employee.job_title ??
      employee.title ??
      employee.position ??
      "",

    email: employee.email ?? "",

    phone:
      employee.phone ??
      employee.contact ??
      employee.contactInfo ??
      employee.contact_info ??
      "",

    location:
      employee.location ??
      employee.branch ??
      employee.branchLocation ??
      employee.branch_location ??
      "",

    permissions: employee.permissions || {}
  };
}

// ==========================================
// LOAD EMPLOYEES
// ==========================================

async function loadEmployees() {
  const employeeList = getElement("employeeList");

  if (employeeList) {
    employeeList.innerHTML =
      '<tr><td colspan="8">Loading employees...</td></tr>';
  }

  try {
    const data = await apiRequest("/api/staff");

    let records = [];

    if (Array.isArray(data)) {
      records = data;
    } else if (Array.isArray(data.staff)) {
      records = data.staff;
    } else if (Array.isArray(data.employees)) {
      records = data.employees;
    } else if (Array.isArray(data.data)) {
      records = data.data;
    }

    employees = records.map(normalizeEmployee);

    renderEmployees();
    renderAccessEmployees();

    showMessage(
      employees.length
        ? `${employees.length} employee record(s) loaded.`
        : "No employees found."
    );

    return employees;
  } catch (error) {
    console.error("Could not load employees:", error);

    if (employeeList) {
      employeeList.innerHTML =
        '<tr><td colspan="8">Employees load nahi huay. API URL, backend aur port check karein.</td></tr>';
    }

    showMessage(`Could not load employees: ${error.message}`, true);
    return [];
  }
}

// ==========================================
// RENDER EMPLOYEE TABLE
// employeeList should be a table <tbody>
// ==========================================

function renderEmployees(list = employees) {
  const employeeList = getElement("employeeList");

  if (!employeeList) {
    console.error(
      'Table body "#employeeList" nahi mila. index.html mein tbody id check karein.'
    );
    return;
  }

  employeeList.innerHTML = "";

  if (!list.length) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");

    cell.colSpan = 8;
    cell.textContent = "No employees found.";
    row.appendChild(cell);
    employeeList.appendChild(row);
    return;
  }

  list.forEach(employee => {
    const row = document.createElement("tr");

    const values = [
      employee.firstName,
      employee.lastName,
      employee.jobTitle,
      employee.email,
      employee.phone,
      employee.location
    ];

    values.forEach(value => {
      const cell = document.createElement("td");
      cell.textContent = value || "—";
      row.appendChild(cell);
    });

    // Access button
    const accessCell = document.createElement("td");
    const accessButton = document.createElement("button");

    accessButton.type = "button";
    accessButton.textContent = "Manage Access";
    accessButton.addEventListener("click", () => {
      const dropdown = getElement("accessEmployee");

      if (dropdown) {
        dropdown.value = String(employee.id);
        dropdown.dispatchEvent(new Event("change"));
      }

      const accessSection = getElement("staffAccess") ||
                            getElement("accessSection");

      if (accessSection) {
        accessSection.scrollIntoView({
          behavior: "smooth",
          block: "start"
        });
      }
    });

    accessCell.appendChild(accessButton);
    row.appendChild(accessCell);

    // Delete button
    const deleteCell = document.createElement("td");
    const deleteButton = document.createElement("button");

    deleteButton.type = "button";
    deleteButton.textContent = "Delete";
    deleteButton.style.color = "#dc2626";

    deleteButton.addEventListener("click", async () => {
      await deleteEmployee(employee.id, employee.name);
    });

    deleteCell.appendChild(deleteButton);
    row.appendChild(deleteCell);

    employeeList.appendChild(row);
  });
}

// ==========================================
// THREE SEARCH BARS
// IDs: firstSearch, lastSearch, locationSearch
// ==========================================

function filterEmployees() {
  const first = getValue("firstSearch").toLowerCase();
  const last = getValue("lastSearch").toLowerCase();
  const location = getValue("locationSearch").toLowerCase();

  const filtered = employees.filter(employee => {
    const firstName = String(employee.firstName).toLowerCase();
    const lastName = String(employee.lastName).toLowerCase();
    const branch = String(employee.location).toLowerCase();

    return firstName.includes(first) &&
           lastName.includes(last) &&
           branch.includes(location);
  });

  renderEmployees(filtered);
}

function setupSearch() {
  ["firstSearch", "lastSearch", "locationSearch"].forEach(id => {
    const input = getElement(id);

    if (input) {
      input.addEventListener("input", filterEmployees);
    }
  });
}

// ==========================================
// ADD EMPLOYEE
// Form ID: employeeForm
// ==========================================

async function addEmployee(event) {
  if (event) {
    event.preventDefault();
  }

  const firstName = getValue("firstName");
  const lastName = getValue("lastName");
  const jobTitle = getValue("jobTitle");
  const email = getValue("email");

  // Optional fields
  const phone =
    getValue("phone") ||
    getValue("contact") ||
    getValue("contactInfo");

  const location =
    getValue("location") ||
    getValue("branch") ||
    getValue("branchLocation");

  if (!firstName || !lastName || !jobTitle || !email) {
    showMessage(
      "First name, last name, job title aur email fill karein.",
      true
    );
    return;
  }

  const submitButton = getElement("employeeForm")
    ?.querySelector('[type="submit"]');

  if (submitButton) {
    submitButton.disabled = true;
  }

  try {
    const employeeData = {
      firstName,
      lastName,
      name: `${firstName} ${lastName}`,
      jobTitle,
      title: jobTitle,
      email,
      phone,
      contact: phone,
      location,
      branch: location
    };

    await apiRequest("/api/staff", {
      method: "POST",
      body: JSON.stringify(employeeData)
    });

    const form = getElement("employeeForm");

    if (form) {
      form.reset();
    }

    await loadEmployees();
    showMessage("Employee add request successful.");
  } catch (error) {
    console.error("Add employee failed:", error);

    showMessage(
      `Employee add nahi hua: ${error.message}. Backend POST /api/staff check karein.`,
      true
    );
  } finally {
    if (submitButton) {
      submitButton.disabled = false;
    }
  }
}

function setupEmployeeForm() {
  const form = getElement("employeeForm");

  if (form) {
    form.addEventListener("submit", addEmployee);
  } else {
    console.warn('Form "#employeeForm" nahi mila.');
  }
}

// ==========================================
// DELETE EMPLOYEE
// Backend must support DELETE /api/staff/{id}
// ==========================================

async function deleteEmployee(id, name = "this employee") {
  if (id === undefined || id === null || id === "") {
    showMessage(
      "Employee ID missing hai. Backend records mein unique ID zaroori hai.",
      true
    );
    return;
  }

  if (!window.confirm(`Kya aap ${name} ko delete karna chahte hain?`)) {
    return;
  }

  try {
    await apiRequest(`/api/staff/${encodeURIComponent(id)}`, {
      method: "DELETE"
    });

    await loadEmployees();
    showMessage("Employee delete ho gaya.");
  } catch (error) {
    console.error("Delete employee failed:", error);

    showMessage(
      `Employee delete nahi hua: ${error.message}. Backend DELETE route check karein.`,
      true
    );
  }
}

// ==========================================
// STAFF ACCESS DROPDOWN
// ID: accessEmployee
// ==========================================

function renderAccessEmployees() {
  const dropdown = getElement("accessEmployee");

  if (!dropdown) {
    return;
  }

  const previousValue = dropdown.value;

  dropdown.innerHTML = "";

  const placeholder = document.createElement("option");
  placeholder.value = "";
  placeholder.textContent = "Select an employee";
  dropdown.appendChild(placeholder);

  employees.forEach(employee => {
    const option = document.createElement("option");

    option.value = String(employee.id);
    option.textContent =
      `${employee.name} — ${employee.jobTitle || "Staff"}`;

    dropdown.appendChild(option);
  });

  if (
    previousValue &&
    employees.some(employee => String(employee.id) === previousValue)
  ) {
    dropdown.value = previousValue;
  }

  loadEmployeePermissions();
}

function findSelectedEmployee() {
  const dropdown = getElement("accessEmployee");

  if (!dropdown || !dropdown.value) {
    return null;
  }

  return employees.find(
    employee => String(employee.id) === dropdown.value
  ) || null;
}

// ==========================================
// PERMISSIONS
// Checkbox IDs:
// canViewCustomers, canManageAccounts,
// canViewReports, canManageStaff,
// canAddEmployees, canDeleteEmployees
// ==========================================

function loadEmployeePermissions() {
  const employee = findSelectedEmployee();

  PERMISSION_IDS.forEach(id => {
    const checkbox = getElement(id);

    if (!checkbox) {
      return;
    }

    const key = id.replace(/^can/, "");
    const lowerKey = key.charAt(0).toLowerCase() + key.slice(1);

    const permissions = employee?.permissions || {};

    checkbox.checked = Boolean(
      permissions[id] ??
      permissions[lowerKey] ??
      permissions[key] ??
      false
    );
  });
}

async function saveEmployeePermissions() {
  const employee = findSelectedEmployee();

  if (!employee) {
    showMessage("Pehle employee select karein.", true);
    return;
  }

  const permissions = {};

  PERMISSION_IDS.forEach(id => {
    const checkbox = getElement(id);

    permissions[id] = checkbox ? checkbox.checked : false;
  });

  try {
    await apiRequest(
      `/api/staff/${encodeURIComponent(employee.id)}/permissions`,
      {
        method: "POST",
        body: JSON.stringify({ permissions })
      }
    );

    employee.permissions = permissions;
    showMessage("Staff permissions save ho gayi hain.");
  } catch (error) {
    console.error("Saving permissions failed:", error);

    showMessage(
      `Permissions save nahi huin: ${error.message}. Backend permissions route check karein.`,
      true
    );
  }
}

function setupPermissions() {
  const dropdown = getElement("accessEmployee");

  if (dropdown) {
    dropdown.addEventListener("change", loadEmployeePermissions);
  }

  const saveButton =
    getElement("savePermissions") ||
    getElement("saveAccess") ||
    getElement("savePermissionsBtn");

  if (saveButton) {
    saveButton.addEventListener("click", saveEmployeePermissions);
  }
}

// ==========================================
// STATUS CHECK
// Optional backend route: GET /api/status
// ==========================================

async function checkBackendStatus() {
  try {
    const status = await apiRequest("/api/status");
    console.log("Backend status:", status);
    return status;
  } catch (error) {
    console.warn("Backend status check failed:", error.message);
    return null;
  }
}

// ==========================================
// INITIALIZE PORTAL
// ==========================================

async function initializePortal() {
  console.log("Bank Admin Portal starting...");

  setupSearch();
  setupEmployeeForm();
  setupPermissions();

  await checkBackendStatus();
  await loadEmployees();

  console.log("Bank Admin Portal initialization finished.");
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initializePortal, {
    once: true
  });
} else {
  initializePortal();
}
