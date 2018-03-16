<?php
// вся процедура работает на сесиях. Именно в ней хранятся данные пользователя, пока он находится на сайте
    session_start();
?>
<html>

<!-- DONE -->

<head>
    <title>Главная страница</title>
</head>

<body>
    <h1 align="center"><b>Главная страница</b></h1>

    <p align="center">
        <IMG SRC=img/banner.png WIDTH=380 HEIGHT=100>
    </p>

    <form action="testreg.php" method="post">
        <!--**** данные из полей отправятся на страничку testreg.php методом "post" ***** -->
        <p align = "center">
            <label>Login:<br></label>
            <input name="login" type="text" <?php if (!(empty($_SESSION['login']) or empty($_SESSION['id']))) echo "disabled" ?> size="15" maxlength="15">
        </p>

        <p align = "center">
            <label>Password:<br></label>
            <input name="password" type="password" <?php if (!(empty($_SESSION['login']) or empty($_SESSION['id']))) echo "disabled" ?> size="16" maxlength="16">
        </p>

        <p align = "center">
        <input type="submit" name="submit" <?php if (!(empty($_SESSION['login']) or empty($_SESSION['id']))) echo "disabled" ?> value="log in">
        <!--**** Кнопочка (type="submit") отправляет данные на страничку testreg.php ***** -->
        <br>

        <a href="reg.php">На страницу регистрации</a>
        </p>
    </form>

    <HR>

    <?php
        echo "<p align = \"center\" >";
        // Проверяем, пусты ли переменные логина и id пользователя
        if (empty($_SESSION['login']) or empty($_SESSION['id']))
            // Если пусты, то мы не выводим ссылку
            echo "Вы вошли на сайт как гость";
        else
            // Если не пусты, то мы выводим ссылку
            echo "Вы вошли на сайт как <i>".$_SESSION['login']."</i><br><a href='\logout.php'>Выйти</a>";
        echo "</p>"
    ?>

</body>
</html>
