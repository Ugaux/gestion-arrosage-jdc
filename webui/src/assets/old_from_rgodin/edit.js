
document.addEventListener("DOMContentLoaded", function(event) { 
    // Select all optionButtons with name = mode
    const checkbox = document.querySelectorAll("[name='mode']");
    
    // Add event listener (onchange) and show days only if custom optionButton is checked
    checkbox.forEach(function(isChecked) {
        isChecked.addEventListener("change", function() {
            if (document.getElementById('custom').checked) {
                document.getElementById("custom_days").removeAttribute("hidden");
            } else {
                document.getElementById("custom_days").setAttribute("hidden", true);
            }
        });
    });
});


// Start time validation
function isTime(time) {
    const isValid = /^([0-1][0-9]|2[0-3]):([0-5][0-9])$/.test(time.value);
    if (isValid) {
        time.style.color = 'black';
        document.getElementById("btn_done").style.pointerEvents = "auto";
    } else {
        time.style.color = "red"
        document.getElementById("btn_done").style.pointerEvents = "none";
    }
    return isValid;
}

// Duration validation
function isDuration(duration) {
    const isValid = /^([1-9]|[1-9][0-9]|[1][0-1][0-9]|120)$/.test(duration.value);
    if (isValid) {
        duration.style.color = 'black';
        document.getElementById("btn_done").style.pointerEvents = "auto";
    } else {
        duration.style.color = "red"
        document.getElementById("btn_done").style.pointerEvents = "none";
    }
    return isValid;
}

// Submit Form
function submit() {
    document.getElementById("update_form").submit();
}

// Delete a watering from a way by schedule Id
function remove() {
    let url = "/remove?way=" + document.getElementById("way").value + "&schedule=" + document.getElementById("schedule").value;
    document.location.href=url;
}
    
