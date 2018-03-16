<html>

<!-- DONE -->

<head>
    <title>Регистрация</title>
</head>


<body>
    <h1 align="center">Регистрация</h1>
    <p align="center">
        <IMG SRC=img/reg.png HEIGHT=100>
    </p>

    <form action="save_user.php" method="post">
        <!-- save_user.php - это адрес обработчика -->
        <p align="center">
            <label>Enter login:<br></label>
            <input name="login" type="text" size="15" maxlength="15">
        </p>
        <!-- В текстовое поле (name="login" type="text") пользователь вводит свой логин -->
        <p align="center">
            <label>Enter password:<br></label>
            <input name="password" type="password" size="15" maxlength="15">
        </p>
        <!-- В поле для паролей (name="password" type="password") пользователь вводит свой пароль -->
        <p align="center">
            <input type="submit" name="submit" value="Register">
            <!-- Кнопочка (type="submit") отправляет данные на страничку save_user.php -->
            <HR>
        </p>

        <p align="center">
            <a href="index.php">На страницу входа</a>
        </p>


    </form>
</body>
</html>
