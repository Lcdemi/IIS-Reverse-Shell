<?php
if ($_SERVER["REQUEST_METHOD"] === "POST") {
    $name = htmlspecialchars($_POST['name']);
    $email = htmlspecialchars($_POST['email']);
    $message = htmlspecialchars($_POST['message']);

    // Basic validation
    if (!empty($name) && !empty($email) && !empty($message)) {
        // Log the submission for now (you can replace this with email functionality)
        file_put_contents("contact_log.txt", "Name: $name\nEmail: $email\nMessage: $message\n\n", FILE_APPEND);
        echo "Thank you, $name! Your message has been received.";
    } else {
        echo "Please fill in all fields.";
    }
} else {
    echo "Invalid request method.";
}
?>
