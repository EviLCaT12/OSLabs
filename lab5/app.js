const express = require('express');
const sqlite3 = require('sqlite3').verbose();
const cors = require('cors');

const app = express();
const port = 3000;

app.use(cors());

const db = new sqlite3.Database('temperature_logs.db', (err) => {
  if (err) {
    console.error('Could not connect to database', err);
  } else {
    console.log('Connected to the database');
  }
});

const executeQuery = (query) => {
    return new Promise((resolve, reject) => {
      db.all(query, [], (err, rows) => {
        if (err) {
          reject(err);
        } else {
          resolve(rows);
        }
      });
    });
  };

app.get('/getAllData', async (req, res) => {
    try {
        const queryAll = 'SELECT * FROM temperature_all;';
        const queryHour = 'SELECT * FROM temperature_hour;';
        const queryDay = 'SELECT * FROM temperature_day;';
    
        const resultAll = await executeQuery(queryAll);
        const resultHour = await executeQuery(queryHour);
        const resultDay = await executeQuery(queryDay);
    
        const responseData = {
            temperature_all: resultAll,
            temperature_hour: resultHour,
            temperature_day: resultDay,
        };
        res.json(responseData);
    } catch (error) {
        console.error('Error executing queries', error);
        res.status(500).send('Internal Server Error');
    }
});

app.get('/getCurrentTemperature', async (req, res) => {
  try {
      const currentTemperatureQuery = 'SELECT temperature FROM temperature_all ORDER BY timestamp DESC LIMIT 1;';
      const currentTemperatureResult = await executeQuery(currentTemperatureQuery);
      const currentTemperature = currentTemperatureResult[0].temperature;

      const responseData = {
          currentTemperature,
      };

      res.json(responseData);
  } catch (error) {
      console.error('Error executing queries', error);
      res.status(500).send('Internal Server Error');
  }
});

app.get('/getTemperatureData', async (req, res) => {
  try {
      const selectedDate = req.query.selectedDate;
      const selectedDateForQuery = selectedDate.replaceAll("-", "/");
      const temperatureDataQuery = `
        SELECT id, timestamp, temperature
        FROM temperature_all 
        WHERE timestamp
        > '${selectedDateForQuery} 00:00:00'
        AND timestamp < '${selectedDateForQuery} 23:59:59';`;

      const temperatureDataResult = await executeQuery(temperatureDataQuery);

      const temperatureData = temperatureDataResult.map(entry => ({
          id: entry.id,
          timestamp: entry.timestamp,
          temperature: entry.temperature,
      }));

      const chartLabels = temperatureData.map(entry => entry.timestamp);
        const chartData = temperatureData.map(entry => entry.temperature);

      const responseData = {
          temperatureData,
          chartLabels,
          chartData,
      };

      res.json(responseData);
  } catch (error) {
      console.error('Error executing queries', error);
      res.status(500).send('Internal Server Error');
  }
});

app.get('/getPeriodTemperatureData', async (req, res) => {
  try {
      const selectedStartDate = req.query.selectedStartDate.replaceAll("-", "/").replaceAll("T", " ");
      const selectedEndDate = req.query.selectedEndDate.replaceAll("-", "/").replaceAll("T", " ");

      const temperatureDataQuery = `
        SELECT id, timestamp, temperature
        FROM temperature_all 
        WHERE timestamp
        > '${selectedStartDate}:00'
        AND timestamp < '${selectedEndDate}:00';`;

      const temperatureDataResult = await executeQuery(temperatureDataQuery);

      const temperatureData = temperatureDataResult.map(entry => ({
          id: entry.id,
          timestamp: entry.timestamp,
          temperature: entry.temperature,
      }));

      const chartLabels = temperatureData.map(entry => entry.timestamp);
        const chartData = temperatureData.map(entry => entry.temperature);

      const responseData = {
          temperatureData,
          chartLabels,
          chartData,
      };

      res.json(responseData);
  } catch (error) {
      console.error('Error executing queries', error);
      res.status(500).send('Internal Server Error');
  }
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});