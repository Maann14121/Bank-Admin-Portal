
const form = document.getElementById("employeeForm");
const list = document.getElementById("employeeList");
const message = document.getElementById("message");

const fields = {
  first: document.getElementById("firstSearch"),
  last: document.getElementById("lastSearch"),
  location: document.getElementById("locationSearch")
};

let employees = [];

function renderEmployees() {
  const first = fields.first.value.trim().toLowerCase();
  const last = fields.last.value.trim().toLowerCase();
  const location = fields.location.value.trim().toLowerCase();

  list.replaceChildren();

  employees.forEach((employee, index) => {
    if (!employee.first.toLowerCase().includes(first)) return;
    if (!employee.last.toLowerCase().includes(last)) return;
    if (!employee.location.toLowerCase().includes(location)) return;

    const row = document.createElement("tr");

    [
      employee.first + " " + employee.last,
      employee.title,
      employee.email,
      employee.phone,
      employee.location
    ].forEach(value => {
      const cell = document.createElement("td");
      cell.textContent = value;
      row.appendChild(cell);
    });

    const actionCell = document.createElement("td");
    const removeButton = document.createElement("button");

    removeButton.textContent = "Remove";
    removeButton.className = "delete-button";
    removeButton.type = "button";

    removeButton.addEventListener("click", () => {
      employees.splice(index, 1);
      renderEmployees();
      message.textContent = "Employee removed.";
    });

    actionCell.appendChild(removeButton);
    row.appendChild(actionCell);
    list.appendChild(row);
  });
}

form.addEventListener("submit", event => {
  event.preventDefault();

  employees.push({
    first: document.getElementById("firstName").value.trim(),
    last: document.getElementById("lastName").value.trim(),
    title: document.getElementById("jobTitle").value.trim(),
    email: document.getElementById("email").value.trim(),
    phone: document.getElementById("phone").value.trim(),
    location: document.getElementById("location").value.trim()
  });

  form.reset();
  renderEmployees();
  message.textContent = "Employee added successfully.";
});

Object.values(fields).forEach(input => {
  input.addEventListener("input", renderEmployees);
});
