document.addEventListener("DOMContentLoaded", function() {
    const button = document.getElementById("changeTextBtn");
    const text = document.getElementById("dynamicText");

    button.addEventListener("click", function() {
        text.innerText = "You clicked the button! 🚀";
        text.style.color = "green";
    });
});
