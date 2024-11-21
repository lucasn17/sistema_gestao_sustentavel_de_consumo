<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="refresh" content="5" >
    <link rel="stylesheet" type="text/css" href="style.css" media="screen"/>
    <title> Sistema de Aquisição de Dados - Chuveiro Elétrico </title>
</head>

<body>
    <h1>Sistema de Aquisição de Dados - Chuveiro Elétrico</h1>
    <?php
    $servername = "localhost";
    $username = "root";
    $password = "";
    $dbname = "table_shower";

    // Create connection
    $conn = new mysqli($servername, $username, $password, $dbname);
    // Check connection
    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }

    $sql = "SELECT card_id, location, value1, value2, value3, value4, reading_time FROM sensordata ORDER BY id DESC"; /*select items to display from the sensordata table in the data base*/

    echo '<table cellspacing="5" cellpadding="5">
          <tr> 
            <th>ID do Cartão RFID</th> 
            <th>Data $ Hora</th> 
            <th>Local</th> 
            <th>Volume (L)</th> 
            <th>Energia elétrica (KWh)</th>
            <th>Tempo (Seg.)</th> 
            <th>Valor Total (R$)</th>		
          </tr>';
     
    if ($result = $conn->query($sql)) {
        while ($row = $result->fetch_assoc()) {
            $row_card_id = $row["card_id"];
            $row_reading_time = $row["reading_time"];
            $row_location = $row["location"];
            $row_value1 = $row["value1"];
            $row_value2 = $row["value2"]; 
            $row_value3 = $row["value3"];
            $row_value4 = $row["value4"];
            
            // Formatar a data para DIA-MÊS-ANO
            $formatted_reading_time = date("d-m-Y H:i:s", strtotime($row_reading_time));

            echo '<tr> 
                    <td>' . $row_card_id . '</td> 
                    <td>' . $formatted_reading_time . '</td> 
                    <td>' . $row_location . '</td> 
                    <td>' . $row_value1 . '</td> 
                    <td>' . $row_value2 . '</td>
                    <td>' . $row_value3 . '</td> 
                    <td>' . $row_value4 . '</td>				
                  </tr>';
        }
        $result->free();
    }

    $conn->close();
    ?> 
    </table>
</body>
</html>