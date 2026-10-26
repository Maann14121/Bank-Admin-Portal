
const API_BASE = "https://supernatural-spider-p79g9gq7jg9jc6q6p-8080.app.github.dev/api/staff";

let employees = [];

const firstNameInput = document.querySelector(
  '#firstNameSearch, input[placeholder*="First Name" i], input[placeholder*="first name" i]'
);

const lastNameInput = document.querySelector(
  '#lastNameSearch, input[placeholder*="Last Name" i], input[placeholder*="last name" i]'
);

const locationInput = document.querySelector(
  '#locationSearch, input[placeholder*="Location" i], input[placeholder*="location" i]'
);

const tableBody =
  document.querySelector("#employeeTableBody") ||
  document.querySelector("#employeeTable tbody") ||
  document.querySelector("table tbody");

function renderEmployees() {
  if (!tableBody) {
    console.error("Employee table body nahi mila. index.html check karo.");
    return;
  }

  const first = (firstNameInput?.value || "").trim().toLowerCase();
  const last = (lastNameInput?.value || "").trim().toLowerCase();
  const location = (locationInput?.value || "").trim().toLowerCase();

  const filtered = employees.filter(employee =>
    (employee.firstName || "").toLowerCase().includes(first) &&
    (employee.lastName || "").toLowerCase().includes(last) &&
    (employee.branch || "").toLowerCase().includes(location)
  );

  tableBody.replaceChildren();

  if (filtered.length === 0) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 6;
    cell.textContent = "No employees found.";
    row.appendChild(cell);
    tableBody.appendChild(row);
    return;
  }

  filtered.forEach(employee => {
    const row = document.createElement("tr");

    [
      employee.firstName,
      employee.lastName,
      employee.title,
      employee.email,
      employee.phone,
      employee.branch
    ].forEach(value => {
      const cell = document.createElement("td");
      cell.textContent = value || "";
      row.appendChild(cell);
    });

    tableBody.appendChild(row);
  });
}

async function loadEmployees() {
  if (tableBody) {
    tableBody.textContent = "Loading staff records...";
  }

  try {
    const response = await fetch(`${API_BASE}/api/staff`);

    if (!response.ok) {
      throw new Error(`Server error: ${response.status}`);
    }

    employees = await response.json();
    renderEmployees();
  } catch (error) {
    console.error("Staff records load nahi huay:", error);

    if (tableBody) {
      tableBody.textContent =
        "Records load nahi huay. Backend aur API URL check karo.";
    }
  }
}

[firstNameInput, lastNameInput, locationInput].forEach(input => {
  input?.addEventListener("input", renderEmployees);
});

loadEmployees();
