%H diorthosi ton varon ginetai sto telos kathe epoxhs
clear all
lamda=0.8;
b=1;
epoxes=2000;
[x,y]=iris_dataset;
leng=length(x);
%Initialization of weight matrices%
W1=rand(4,4);
W2=rand(5,4);
W3=rand(3,5);
J=zeros(epoxes,1);

for i=1:epoxes
    J_element1=zeros();
    J_element2=zeros();
    J_element3=zeros();
    correct=0;
    for j=1:2:leng%misa test
        %%%%--h=1/e^-bx--%%%%
        u1=x(:,j);
        v1=W1*u1;
        u2=1./(1+exp(-v1));
        v2=W2*u2;
        u3=1./(1+exp(-v2));
        v3=W3*u3;
        u4=1./(1+exp(-v3));
        d=y(:,j);
        e=d-u4;
        Jt=e.'*e;
        J(i)=J(i)+Jt;
        hdiv3=exp(-v3)./((exp(-v3)+1).*(1+exp(-v3)));
        delta3=e.*hdiv3;
        hdiv2=exp(-v2)./((exp(-v2)+1).*(1+exp(-v2)));
        delta2=hdiv2.*(W3'*delta3);
        hdiv1=exp(-v1)./((exp(-v1)+1).*(1+exp(-v1)));
        delta1=hdiv1.*(W2'*delta2);
        element1=-u1*delta1';
        element2=-u2*delta2';
        element3=-u3*delta3';
        J_element1=J_element1+element1;
        J_element2=J_element2+element2;
        J_element3=J_element3+element3;
    end
    %Fix Gradients
    J_element1=J_element1./max(abs(J_element1(:)));
    J_element2=J_element2./max(abs(J_element2(:)));
    J_element3=J_element3./max(abs(J_element3(:)));
    W1=W1-(lamda.*J_element1);
    W2=W2-(lamda.*J_element2');
    W3=W3-(lamda.*J_element3');
end
%plot
figure(1)
plot(J);
xlabel('Εποχή');
ylabel('J');
title('Καμπύλη Εκμάθησης');