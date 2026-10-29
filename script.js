
"use strict";

// BANK ADMIN PORTAL - FRONTEND

const API_BASE = "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

let employees = [];

const PERMISSION_IDS = [
  "canViewCustomers",
  "canManageAccounts",
  "canViewReports",
  "canManageStaff",
  "canAddEmployees",
  "canDeleteEmployees"
];

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

function normalizeEmployee(employee, index) {
  const firstName = employee.firstName ?? employee.first_name ?? "";
  const lastName = employee.lastName ?? employee.last_name ?? "";

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

    permissions: {
      canViewCustomers: employee.permissions?.canViewCustomers ?? employee.canViewCustomers ?? false,
      canManageAccounts: employee.permissions?.canManageAccounts ?? employee.canManageAccounts ?? false,
      canViewReports: employee.permissions?.canViewReports ?? employee.canViewReports ?? false,
      canManageStaff: employee.permissions?.canManageStaff ?? employee.canManageStaff ?? false,
      canAddEmployees: employee.permissions?.canAddEmployees ?? employee.canAddEmployees ?? false,
      canDeleteEmployees: employee.permissions?.canDeleteEmployees ?? employee.canDeleteEmployees ?? false
    }
  };
}

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

function renderEmployees(list = employees) {
  const employeeList = getElement("employeeList");

  if (!employeeList) {
    console.error('Table body "#employeeList" nahi mila.');
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

      const accessSection =
        getElement("staffAccess") ||
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

async function addEmployee(event) {
  if (event) event.preventDefault();

  const firstName = getValue("firstName");
  const lastName = getValue("lastName");
  const jobTitle = getValue("jobTitle");
  const email = getValue("email");

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

  if (submitButton) submitButton.disabled = true;

  try {
    const formData = new URLSearchParams({
      firstName,
      lastName,
      title: jobTitle,
      email,
      phone: phone || "",
      branch: location || ""
    });

    await apiRequest("/api/staff", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body: formData.toString()
    });

    const form = getElement("employeeForm");
    if (form) form.reset();

    await loadEmployees();
    showMessage("Employee successfully add ho gaya.");
  } catch (error) {
    console.error("Add employee failed:", error);
    showMessage(`Employee add nahi hua: ${error.message}`, true);
  } finally {
    if (submitButton) submitButton.disabled = false;
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

async function deleteEmployee(id, name = "this employee") {
  if (id === undefined || id === null || id === "") {
    showMessage("Employee ID missing hai.", true);
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
    showMessage(`Employee delete nahi hua: ${error.message}`, true);
  }
}

function renderAccessEmployees() {
  const dropdown = getElement("accessEmployee");
  if (!dropdown) return;

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
  if (!dropdown || !dropdown.value) return null;

  return employees.find(
    employee => String(employee.id) === dropdown.value
  ) || null;
}

function loadEmployeePermissions() {
  const employee = findSelectedEmployee();

  PERMISSION_IDS.forEach(id => {
    const checkbox = getElement(id);
    if (!checkbox) return;

    const permissions = employee?.permissions || {};

    const value =
      permissions[id] ??
      employee?.[id] ??
      false;

    checkbox.checked =
      value === true || value === 1 || value === "1";
  });
}

async function saveEmployeePermissions() {
  const employee = findSelectedEmployee();

  if (!employee) {
    showMessage("Pehle employee select karein.", true);
    return;
  }

  const employeeId = employee.id;
  const formData = new URLSearchParams();

  PERMISSION_IDS.forEach(id => {
    const checkbox = getElement(id);
    formData.set(id, checkbox && checkbox.checked ? "1" : "0");
  });

  try {
    await apiRequest(
      `/api/staff/${encodeURIComponent(employeeId)}/permissions`,
      {
        method: "POST",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded"
        },
        body: formData.toString()
      }
    );

    await loadEmployees();

    const dropdown = getElement("accessEmployee");
    if (dropdown) dropdown.value = String(employeeId);

    loadEmployeePermissions();
    showMessage("Staff permissions save ho gayi hain.");
  } catch (error) {
    console.error("Saving permissions failed:", error);
    showMessage(`Permissions save nahi huin: ${error.message}`, true);
  }
}

function setupPermissions() {
  const permissionsForm = getElement("permissionsForm");
  if (permissionsForm) permissionsForm.addEventListener("submit", event => { event.preventDefault(); saveEmployeePermissions(); });

  const dropdown = getElement("accessEmployee");

  if (dropdown) {
    dropdown.addEventListener("change", loadEmployeePermissions);
  }

}

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

async function initializePortal() {
  console.log("Bank Admin Portal starting...");

  setupSearch();
  setupEmployeeForm();
  setupPermissions();
  setupCustomers();

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


// ---------------- CUSTOMER / SERVICE USER MANAGEMENT ----------------
function customerMessage(text, isError = false) {
  const el = getElement("customerMessage");
  if (!el) return;
  el.textContent = text;
  el.style.color = isError ? "#dc2626" : "#15803d";
}

function formatMoney(value) {
  const amount = Number(value);
  if (!Number.isFinite(amount)) return String(value ?? "0");
  return amount.toLocaleString("en-PK", { minimumFractionDigits: 2, maximumFractionDigits: 2 });
}

function renderCustomer(customer) {
  const result = getElement("customerResult");
  if (!result) return;
  if (!customer) {
    result.innerHTML = '<p class="muted">CNIC se customer search karein.</p>';
    return;
  }

  const active = customer.active === true || customer.active === 1 || customer.active === "1";
  result.innerHTML = `
    <div class="customer-details">
      <h3>${escapeHtml(customer.fullName)}</h3>
      <p><strong>CNIC:</strong> ${escapeHtml(customer.cnic)}</p>
      <p><strong>Account Type:</strong> ${escapeHtml(customer.accountType)}</p>
      <p><strong>Account Number:</strong> ${escapeHtml(customer.accountNumber)}</p>
      <p><strong>Balance:</strong> Rs. ${formatMoney(customer.balance)}</p>
      <p><strong>Status:</strong> <span class="${active ? "status-active" : "status-inactive"}">${active ? "Active" : "Inactive"}</span></p>
      <button type="button" id="toggleCustomerStatus">${active ? "Deactivate Account" : "Activate Account"}</button>
    </div>`;
  getElement("toggleCustomerStatus")?.addEventListener("click", () => toggleCustomerStatus(customer.id, !active));
}

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, ch => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[ch]));
}

async function searchCustomer(event) {
  if (event) event.preventDefault();
  const cnic = getValue("customerCnic");
  if (!cnic) {
    customerMessage("Customer ka CNIC enter karein.", true);
    return;
  }
  try {
    const results = await apiRequest(`/api/customers?cnic=${encodeURIComponent(cnic)}`);
    if (!Array.isArray(results) || results.length === 0) {
      renderCustomer(null);
      customerMessage("Is CNIC par customer nahi mila.", true);
      return;
    }
    renderCustomer(results[0]);
    customerMessage("Customer record mil gaya.");
  } catch (error) {
    customerMessage(`Search nahi ho saki: ${error.message}`, true);
  }
}

async function toggleCustomerStatus(id, active) {
  const body = new URLSearchParams({ active: active ? "1" : "0" });
  try {
    const updated = await apiRequest(`/api/customers/${encodeURIComponent(id)}/status`, {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    });
    renderCustomer(updated);
    customerMessage(`Account ${active ? "activate" : "deactivate"} ho gaya aur save ho gaya.`);
  } catch (error) {
    customerMessage(`Status update nahi hua: ${error.message}`, true);
  }
}

async function addCustomer(event) {
  if (event) event.preventDefault();
  const cnic = getValue("newCustomerCnic");
  const fullName = getValue("customerFullName");
  const accountType = getValue("accountType");
  const accountNumber = getValue("accountNumber");
  const balance = getValue("customerBalance");

  if (!cnic || !fullName || !accountType || !accountNumber || !balance) {
    customerMessage("Customer ki tamam fields fill karein.", true);
    return;
  }

  try {
    const body = new URLSearchParams({ cnic, fullName, accountType, accountNumber, balance });
    const created = await apiRequest("/api/customers", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    });
    getElement("customerForm")?.reset();
    getElement("customerCnic").value = created.cnic;
    renderCustomer(created);
    customerMessage("Customer successfully add ho gaya.");
  } catch (error) {
    customerMessage(`Customer add nahi hua: ${error.message}`, true);
  }
}

function setupCustomers() {
  getElement("customerSearchForm")?.addEventListener("submit", searchCustomer);
  getElement("customerForm")?.addEventListener("submit", addCustomer);
}
