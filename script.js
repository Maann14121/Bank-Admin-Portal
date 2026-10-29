
const API_BASE =
  "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev";

// Existing page elements
const employeeList = document.getElementById("employeeList");
const message = document.getElementById("message");
const employeeForm = document.getElementById("employeeForm");

const firstSearch = document.getElementById("firstSearch");
const lastSearch = document.getElementById("lastSearch");
const locationSearch = document.getElementById("locationSearch");

// Access and permissions elements
const accessEmployeeSelect = document.getElementById("accessEmployee");
const permissionsForm = document.getElementById("permissionsForm");
const permissionsMessage = document.getElementById("permissionsMessage");

const accessPermissionFields = [
  "canViewCustomers",
  "canManageAccounts",
  "canViewReports",
  "canManageStaff",
  "canAddEmployees",
  "canDeleteEmployees"
];

let employees = [];

function showMessage(element, text) {
  if (element) element.textContent = text;
}

function escapeHTML(value) {
  return String(value ?? "").replace(/[&<>"']/g, character => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  })[character]);
}

// Render employee directory and apply search filters
function renderEmployees() {
  if (!employeeList) return;

  const first = firstSearch?.value.trim().toLowerCase() || "";
  const last = lastSearch?.value.trim().toLowerCase() || "";
  const location = locationSearch?.value.trim().toLowerCase() || "";

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
        <button type="button"
          class="delete-employee"
          data-id="${escapeHTML(employee.id)}">
          Delete
        </button>
      </td>
    </tr>
  `).join("");
}

// Populate the employee dropdown from the API data
function renderAccessEmployees() {
  if (!accessEmployeeSelect) {
    console.error("Dropdown #accessEmployee HTML mein nahi mila.");
    return;
  }

  const selectedId = accessEmployeeSelect.value;

  accessEmployeeSelect.replaceChildren();

  const placeholder = document.createElement("option");
  placeholder.value = "";
  placeholder.textContent = "Select an employee";
  accessEmployeeSelect.appendChild(placeholder);

  employees.forEach(employee => {
    const option = document.createElement("option");
    option.value = String(employee.id);
    option.textContent =
      `${employee.firstName} ${employee.lastName} (${employee.title})`;

    accessEmployeeSelect.appendChild(option);
  });

  if (employees.some(employee => String(employee.id) === selectedId)) {
    accessEmployeeSelect.value = selectedId;
  }

  console.log(
    "Access dropdown employees:",
    accessEmployeeSelect.options.length - 1
  );
}

// Load employee data once and update both sections
async function loadEmployees() {
  try {
    showMessage(message, "Loading employees...");

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
    renderAccessEmployees();

    showMessage(message, `${employees.length} employees loaded.`);
  } catch (error) {
    console.error("Load employees error:", error);
    showMessage(message, `Could not load employees: ${error.message}`);
    showMessage(
      permissionsMessage,
      `Employees load nahi huay: ${error.message}`
    );
  }
}

// ADD EMPLOYEE
if (employeeForm) {
  employeeForm.addEventListener("submit", async event => {
    event.preventDefault();

    const submitButton =
      employeeForm.querySelector('button[type="submit"]');

    const firstName = document.getElementById("firstName");
    const lastName = document.getElementById("lastName");
    const jobTitle = document.getElementById("jobTitle");
    const email = document.getElementById("email");
    const phone = document.getElementById("phone");
    const location = document.getElementById("location");

    if (!firstName || !lastName || !jobTitle || !email || !phone || !location) {
      showMessage(message, "Employee form ke required fields nahi milay.");
      return;
    }

    const formData = new URLSearchParams({
      firstName: firstName.value.trim(),
      lastName: lastName.value.trim(),
      title: jobTitle.value.trim(),
      email: email.value.trim(),
      phone: phone.value.trim(),
      branch: location.value.trim()
    });

    try {
      if (submitButton) submitButton.disabled = true;
      showMessage(message, "Saving employee...");

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

      showMessage(
        message,
        `Employee ${result.firstName ?? ""} ${result.lastName ?? ""} added successfully.`
      );
    } catch (error) {
      console.error("Add employee error:", error);
      showMessage(message, `Could not add employee: ${error.message}`);
    } finally {
      if (submitButton) submitButton.disabled = false;
    }
  });
}

// DELETE EMPLOYEE
if (employeeList) {
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
      showMessage(message, "Deleting employee...");

      const response = await fetch(
        `${API_BASE}/api/staff/${encodeURIComponent(id)}`,
        { method: "DELETE" }
      );

      const result = await response.json();

      if (!response.ok) {
        throw new Error(result.error || `HTTP ${response.status}`);
      }

      await loadEmployees();

      showMessage(
        message,
        `${employee.firstName} ${employee.lastName} deleted successfully.`
      );
    } catch (error) {
      console.error("Delete employee error:", error);
      showMessage(message, `Could not delete employee: ${error.message}`);
    }
  });
}

// SEARCH BARS
[firstSearch, lastSearch, locationSearch].forEach(input => {
  if (input) input.addEventListener("input", renderEmployees);
});

// Clear all permission checkboxes
function clearPermissionCheckboxes() {
  accessPermissionFields.forEach(field => {
    const checkbox = document.getElementById(field);
    if (checkbox) checkbox.checked = false;
  });
}

// Select an employee and show their permissions
if (accessEmployeeSelect) {
  accessEmployeeSelect.addEventListener("change", () => {
    const employee = employees.find(
      item => String(item.id) === accessEmployeeSelect.value
    );

    clearPermissionCheckboxes();

    if (!employee) {
      showMessage(permissionsMessage, "");
      return;
    }

    accessPermissionFields.forEach(field => {
      const checkbox = document.getElementById(field);

      if (!checkbox) {
        console.error(`Missing permission checkbox: ${field}`);
        return;
      }

      checkbox.checked =
        employee[field] === true ||
        employee[field] === 1 ||
        employee[field] === "1";
    });

    showMessage(
      permissionsMessage,
      `Permissions loaded for ${employee.firstName} ${employee.lastName}.`
    );
  });
}

// SAVE PERMISSIONS
if (permissionsForm) {
  permissionsForm.addEventListener("submit", async event => {
    event.preventDefault();

    const employeeId = accessEmployeeSelect?.value;

    if (!employeeId) {
      showMessage(permissionsMessage, "Pehle employee select karo.");
      return;
    }

    const permissions = {};

    for (const field of accessPermissionFields) {
      const checkbox = document.getElementById(field);

      if (!checkbox) {
        showMessage(
          permissionsMessage,
          `HTML mein ${field} checkbox nahi mila.`
        );
        return;
      }

      permissions[field] = checkbox.checked ? "1" : "0";
    }

    const saveButton =
      permissionsForm.querySelector('button[type="submit"]');

    try {
      if (saveButton) saveButton.disabled = true;
      showMessage(permissionsMessage, "Saving permissions...");

      const response = await fetch(
        `${API_BASE}/api/staff/${encodeURIComponent(employeeId)}/permissions`,
        {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded"
          },
          body: new URLSearchParams(permissions).toString()
        }
      );

      const result = await response.json();

      if (!response.ok) {
        throw new Error(result.error || `HTTP ${response.status}`);
      }

      await loadEmployees();

      if (accessEmployeeSelect) {
        accessEmployeeSelect.value = employeeId;
        accessEmployeeSelect.dispatchEvent(new Event("change"));
      }

      showMessage(permissionsMessage, "Permissions successfully saved.");
    } catch (error) {
      console.error("Save permissions error:", error);
      showMessage(
        permissionsMessage,
        `Permissions save nahi huin: ${error.message}`
      );
    } finally {
      if (saveButton) saveButton.disabled = false;
    }
  });
}

// Initial page load
loadEmployees();
