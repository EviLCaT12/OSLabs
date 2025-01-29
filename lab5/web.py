from flask import Flask, render_template, request
import requests
import matplotlib.pyplot as plt
from io import BytesIO
import base64

app = Flask(__name__)

TEMP_SERVER_URL = "http://localhost:8080"

@app.route('/', methods=['GET', 'POST'])
def dashboard():
    start_time = request.form.get('start_datetime', '1970-01-01T00:00:00')  
    end_time = request.form.get('end_datetime', '2100-01-01T00:00:00')   
    
    print(start_time)
    print(end_time)
    
    try:
        latest_temp_response = requests.get(f"{TEMP_SERVER_URL}/temperature")
        latest_temp = None
        latest_timestamp = None
        if latest_temp_response.status_code == 200:
            latest_temp_data = latest_temp_response.json()
            latest_temp = latest_temp_data.get('temperature')
            latest_timestamp = latest_temp_data.get('timestamp')
        else:
            latest_temp = "Не удалось получить последнюю температуру"
            latest_timestamp = "Не удалось получить время"
        
        params = {'start_datetime': start_time, 'end_datetime': end_time}
        history_response = requests.get(f"{TEMP_SERVER_URL}/history", params=params)
        
        if history_response.status_code == 200:
            temp_data = history_response.json()
            timestamps = [entry['timestamp'] for entry in temp_data]
            temps = [entry['temperature'] for entry in temp_data]
            
            plt.figure(figsize=(10, 5))
            plt.plot(timestamps, temps, marker='o', linestyle='-', color='b')
            plt.xticks(rotation=45)
            plt.xlabel('Время')
            plt.ylabel('Температура')
            plt.title(f'Температура с {start_time} по {end_time}')
            
            img_buffer = BytesIO()
            plt.tight_layout()
            plt.savefig(img_buffer, format='png')
            img_buffer.seek(0)
            graph_url = base64.b64encode(img_buffer.getvalue()).decode('utf-8')
            
            return render_template('index.html', data=temp_data, graph