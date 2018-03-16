<?php
    session_start();
    if(!isset($_SESSION['login']))
       header("location:index.php");
    else{
?>

        <h2 align=center >Welcome, <?php echo $_SESSION['login'];?>! </h2>
        <p align=center>
        <IMG SRC=img/granted.png HEIGHT=100> <br>
        Вы успешно вошли на сайт! <br> <a href='index.php'>Главная страница</a>
        </p>

        <HR>

        <p align=center><a href="logout.php">Выйти</a></p>

<?php
}
?>
