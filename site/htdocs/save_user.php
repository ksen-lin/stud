<!-- DONE -->

<?php
    if (isset($_POST['login'])){
        $login = $_POST['login'];
        if ($login == '')
            unset($login);
    } //заносим введенный пользователем логин в переменную $login, если он пустой, то уничтожаем переменную

    if (isset($_POST['password'])){
        $password=$_POST['password'];
        if ($password =='')
            unset($password);
    }
    //заносим введенный пользователем пароль в переменную $password, если он пустой, то уничтожаем переменную

    if (empty($login) or empty($password)) //если пользователь не ввел логин или пароль, то выдаем ошибку и останавливаем скрипт
        exit ("Вы ввели не всю информацию, венитесь назад и заполните все поля!");

    $login = stripslashes($login);
    $login = htmlspecialchars($login);

    $password = stripslashes($password);
    $password = htmlspecialchars($password);

    //удаляем лишние пробелы
    $login = trim($login);
    $password = trim($password);

    include ("bd.php");

    // проверка на существование пользователя с таким же логином
    $result = mysqli_query($db, "SELECT id FROM users WHERE login='$login'");
    if (!$result){
        printf("Error: %s\n", mysqli_error($db));
        exit();
    }
    $myrow = mysqli_fetch_array($result);
    if (!empty($myrow['id']))
        exit ("Извините, такой логин уже зарегистрирован. Выберите другой.");

    // если такого нет, то сохраняем данные
    $result2 = mysqli_query($db, "INSERT INTO users (login,password) VALUES('$login','$password')");

    // Проверяем, есть ли ошибки
    if ($result2=='TRUE'){
        echo "<IMG SRC=img/success.png HEIGHT=100> <br>";
        echo "Вы успешно зарегистрированы! Теперь вы можете зайти на сайт. <br> <a href='index.php'>На страницу входа</a>";}
    else{
        echo "<IMG SRC=img/failure.png HEIGHT=100> <br>";
        echo "Ошибка регистрации";
        printf("Error: %s\n", mysqli_error($db));
    }
?>