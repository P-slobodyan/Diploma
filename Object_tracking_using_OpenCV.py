import cv2
import sys
 
(major_ver, minor_ver, subminor_ver) = (cv2.__version__).split('.')
 
if __name__ == '__main__' :
 
    # Установка трекера
    tracker_types = ['MEDIANFLOW' ,'KCF', 'MOSSE', 'CSRT']
    scan_num = len(tracker_types)

    for number in range(2,6):
        # Создание нового текстового файла
        log = open(f'Performance_test_crop_{number}.txt', 'w')
        log.write('\t\tFPS\tLOSS RATE\tLOSS TIME\tTrue/False\n')

        for j in range(scan_num):
            tracker_type = tracker_types[j]
            # Установка трекера
            if int(minor_ver) < 3:
                tracker = cv2.Tracker_create(tracker_type)
            else:
                if  tracker_type == 'KCF':
                    tracker = cv2.TrackerKCF_create()
                elif tracker_type == 'MEDIANFLOW':
                    tracker = cv2.legacy.TrackerMedianFlow_create()
                elif tracker_type == 'MOSSE':
                    tracker = cv2.legacy.TrackerMOSSE_create()
                elif tracker_type == "CSRT":
                    tracker = cv2.TrackerCSRT_create()
                else:
                    print("ERROR")
            # Установка доверительного трекера
            tracker_trust = cv2.TrackerCSRT_create()
            # Читать видео
            video = cv2.VideoCapture(f"crop_{number}.mp4")
        
            # Выход, если видео не открыто
            if not video.isOpened():
                print("Не удалось открыть видео")
                sys.exit()
            
            # Считывание первого кадра.
            ok, frame = video.read()
            if not ok:
                print('Невозможно прочитать видеофайл')
                sys.exit()
            
            # Определение начального ограничительного поля
            bbox = (287, 23, 86, 320)
        
            # выбор интересующей области на изображении
            if(j == 0):
                bbox0 = cv2.selectROI(frame, False)
            bbox = bbox0
            # Инициализация трекера с первым кадром и ограничительной рамкой
            ok = tracker.init(frame, bbox)
            ok_trust = tracker_trust.init(frame, bbox)

            i=0                         # Число итераций
            fps0=0                      # Усредненное 
            f = 0                       # Флаг потери обЪекта
            #f_trust = 0                 # 
            time_loss = (str)(None)     # Впремя потери обЪекта (вычисляется не корректно)
            LOSS_RATE = 0               # Число потерь обЪекта
            false_number = 0            # Число отличий от доверительного трекера
            true_number = 0             # Число совпадений с доверительным трекером

            #Настройка сохранения видео файла
            fourcc = cv2.VideoWriter_fourcc('m', 'p', '4', 'v')
            out = cv2.VideoWriter(f'output_{number}_{tracker_type}.mp4', fourcc, 30.0, (1920, 1080))

            timer0 = cv2.getTickCount()
        
            while True:
                i+=1
                # Считывание нового кадра
                ok, frame = video.read()
                if not ok:
                    fps0 = fps0/(i-1)
                    print("среднее число FPS "+(str)(fps0))
                    break
                
                # Запуск таймера
                timer = cv2.getTickCount()
        
                # Обновление трекера
                ok, bbox = tracker.update(frame)
        
                # Рассчитать количество кадров в секунду (FPS)
                fps = cv2.getTickFrequency() / (cv2.getTickCount() - timer)
                fps0 += int(fps)

                # Обновление доверительного трекера
                ok_trust, bbox_trust = tracker_trust.update(frame)

                # Отслеживание точности трекера
                if(ok and ok_trust):
                    if(bbox[0] <= (bbox_trust[0] + bbox_trust[2]/2) and (bbox[0] + bbox[2]) >= (bbox_trust[0] + bbox_trust[2]/2)):
                        if(bbox[1] <= (bbox_trust[1] + bbox_trust[3]/2) and (bbox[1] + bbox[3]) >= (bbox_trust[1] + bbox_trust[3]/2)):
                            true_number += 1
                            #f_trust = 0
                        else:
                            false_number += 1
                            # if(f_trust == 0):
                            #     LOSS_RATE += 1
                            #     f_trust = 1
                    else:
                        false_number += 1
                        # if(f_trust == 0):
                        #     LOSS_RATE += 1
                        #     f_trust = 1
        
                # Нарисовать ограничительную рамку
                if ok:
                    # Отслеживание успеха
                    if(f == 1):
                        f = 0
                    p1 = (int(bbox[0]), int(bbox[1]))
                    p2 = (int(bbox[0] + bbox[2]), int(bbox[1] + bbox[3]))
                    cv2.rectangle(frame, p1, p2, (255,0,0), 2, 1)
                    cv2.putText(frame, "x " + str(int(bbox[0] + bbox[2]/2)) + "; y " + str(int(bbox[1] + bbox[3]/2)), (100,80), cv2.FONT_HERSHEY_SIMPLEX, 0.75,(0,0,255),2)
                else :
                    # Сбой отслеживания
                    if(f == 0):
                        LOSS_RATE += 1
                        time_loss = (str)((cv2.getTickCount() - timer0)/cv2.getTickFrequency())
                        print("среднее число FPS "+(str)(fps0/i))
                        f=1
                    cv2.putText(frame, "Обнаружен сбой отслеживания", (100,80), cv2.FONT_HERSHEY_SIMPLEX, 0.75,(0,0,255),2)
        
                # Отображение типа трекера на рамке
                cv2.putText(frame, tracker_type + " Tracker", (100,20), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (50,170,50),2)
            
                # Отображение FPS на кадре
                cv2.putText(frame, "FPS : " + str(int(fps)), (100,50), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (50,170,50), 2)
        
                # Результат отображения
                cv2.imshow("Tracking", frame)
                out.write(frame)
        
                # Выход при нажатии ESC
                k = cv2.waitKey(1) & 0xff
                if k == 27 : break

            video.release()
            out.release()
            cv2.destroyAllWindows()
            
            # Добавление данных отслеживания в новую строку таблицы
            log.write(str(tracker_type)+'\t')
            log.write(str((int)(fps0))+'\t')          
            log.write(str(LOSS_RATE)+'\t')     
            log.write(str(time_loss)+'\t')     
            log.write((str)(round(true_number/(i-1)*100, 2))+"/"+(str)(round(false_number/(i-1)*100, 2))+'\n')    

        # Сохранение данных в текстовый файл
        log.close()