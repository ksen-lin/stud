<!-- DONE -->
<?php
    session_start();

    if (isset($_POST['login'])){
        $login = $_POST['login'];
        if ($login == '')
            unset($login);
    }
    //заносим введенный пользователем логин в переменную $login, если он пустой, то уничтожаем переменную
    if (isset($_POST['password'])){
        $password=$_POST['password'];
        if ($password =='')
            unset($password);
    }

    if (empty($login) or empty($password)) //если пользователь не ввел логин или пароль, то выдаем ошибку и останавливаем скрипт
        exit("Не введён логин или пароль, заполните все поля и повторите");

    //если логин и пароль введены,то обрабатываем их, чтобы теги и скрипты не работали, мало ли что люди могут ввести
    $login = stripslashes($login);
    $login = htmlspecialchars($login);

    $password = stripslashes($password);
    $password = htmlspecialchars($password);

    //удаляем лишние пробелы
    $login = trim($login);
    $password = trim($password);


    // подключаемся к базе
    include ("bd.php");
    $result = mysqli_query($db, "SELECT * FROM users WHERE login='$login'"); //извлекаем из базы все данные о пользователе с введенным логином
    if (!$result){
        printf("Error: %s\n", mysqli_error($db));
        exit();
    }
    $myrow = mysqli_fetch_array($result);

    if (empty($myrow['password'])){
        echo "<IMG SRC=img/denied.jpg HEIGHT=100> <br>";
        exit("Неверный логин или пароль.");
    }else{
        if ($myrow['password']==$password) {
            //если пароли совпадают, то запускаем пользователю сессию! Можете его поздравить, он вошел!
            $_SESSION['login']=$myrow['login'];
            $_SESSION['id']=$myrow['id'];//эти данные очень часто используются, вот их и будет "носить с собой" вошедший пользователь

//             include("intropage.php");
            $_SESSION['session_username']=$username;
            /* Redirect browser */
            header("Location: intropage.php");


        }else{       //если пароли не сошлись
            echo "<IMG SRC=img/denied.jpg HEIGHT=100> <br>";
            exit ("Неверный логин или пароль");
        }
    }
?>