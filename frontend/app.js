const tbody = document.querySelector("#shotTable tbody");

function addRow(data) {
  const tr = document.createElement("tr");
  tr.classList.add(
    data.classification === 1
      ? "perfect"
      : data.classification === 0
      ? "early"
      : "late"
  );
  tr.innerHTML = `<td>${data.id}</td>
                  <td>${
                    data.classification === 1
                      ? "Perfect"
                      : data.classification === 0
                      ? "Early"
                      : "Late"
                  }</td>
                  <td>${data.scored ? "✅" : "❌"}</td>`;
  tbody.prepend(tr);
}

// load last 20 on start
fetch("/shots")
  .then((r) => r.json())
  .then((json) => json.shots.reverse().forEach(addRow));

const socket = io();
socket.on("shot", addRow);
