<!-- DONE -->

<?php
$db = mysqli_connect('localhost','root','')
        or die("Could not connect: " . mysqli_error());
mysqli_select_db ($db, 'lab');
?>
